#include "v2_notify.hpp"

using namespace V2Notify;

namespace V2Notify
{
    WebSocketsClient webSocket;

    TaskHandle_t v2NotifyTaskHandle;

    String currentHostname;

    bool blockFeeDecimals = DEFAULT_BLOCK_FEE_DECIMALS;
    uint disconnectCount = 0;

    void setupV2Notify()
    {
        String hostname = "ws.btclock.dev";
        blockFeeDecimals = preferences.getBool("blockFeeDec", DEFAULT_BLOCK_FEE_DECIMALS);

        if (getDataSource() == CUSTOM_SOURCE)
        {
            Serial.println(F("Connecting to custom source"));
            hostname = preferences.getString("ceEndpoint", DEFAULT_CUSTOM_ENDPOINT);
            bool useSSL = !preferences.getBool("ceDisableSSL", DEFAULT_CUSTOM_ENDPOINT_DISABLE_SSL);
            
            if (useSSL) {
                webSocket.beginSSL(hostname, 443, "/api/v2/ws");
            } else {
                webSocket.begin(hostname, 80, "/api/v2/ws");
            }
        }
        else
        {
            Serial.println(F("Connecting to V2 source"));
            webSocket.beginSSL(hostname, 443, "/api/v2/ws");
        }

        webSocket.onEvent(V2Notify::onWebsocketV2Event);
        webSocket.setReconnectInterval(5000);
        webSocket.enableHeartbeat(15000, 3000, 2);

        V2Notify::setupV2NotifyTask();

        currentHostname = hostname;
    }

    void onWebsocketV2Event(WStype_t type, uint8_t *payload, size_t length)
    {
        switch (type)
        {
        case WStype_DISCONNECTED:
            disconnectCount++;

            Serial.printf("[WSc] Disconnected! (%d)\n", disconnectCount);
            if (disconnectCount > 1)
            {
                getLedHandler().queueEffect(LED_DATA_BLOCK_ERROR);
            }

            if (disconnectCount > 20)
            {
                // Back off before rebooting to avoid a tight reboot loop when
                // the upstream is unreachable (otherwise we'd brick the device
                // in a crash-restart cycle that only worsens connectivity).
                Serial.println(F("Disconnected too many times, rebooting device after backoff"));
                vTaskDelay(pdMS_TO_TICKS(30 * 1000));
                noInterrupts();
                esp_restart();
                interrupts();
            }
            break;
        case WStype_CONNECTED:
        {
            Serial.print(F("[WSc] Connected to "));
            Serial.print(currentHostname);
            Serial.print(F(": "));
            Serial.println((char *)payload);

            disconnectCount = 0;

            auto sendSubscription = [](JsonDocument &doc) {
                size_t responseLength = measureMsgPack(doc);
                std::unique_ptr<uint8_t[]> buffer(new uint8_t[responseLength]);
                serializeMsgPack(doc, buffer.get(), responseLength);
                webSocket.sendBIN(buffer.get(), responseLength);
            };

            {
                JsonDocument response;
                response["type"] = "subscribe";
                response["eventType"] = blockFeeDecimals ? "blockfee2" : "blockfee";
                sendSubscription(response);
            }
            {
                JsonDocument response;
                response["type"] = "subscribe";
                response["eventType"] = "blockheight";
                sendSubscription(response);
            }
            {
                JsonDocument response;
                response["type"] = "subscribe";
                response["eventType"] = "price";
                JsonArray currenciesArray = response["currencies"].to<JsonArray>();
                for (const auto &str : getActiveCurrencies())
                {
                    currenciesArray.add(str);
                }
                sendSubscription(response);
            }
            break;
        }
        case WStype_TEXT:
            Serial.print(F("[WSc] get text: "));
            Serial.println((char *)payload);

            // send message to server
            // webSocket.sendTXT("message here");
            break;
        case WStype_BIN:
        {
            JsonDocument doc;
            DeserializationError error = deserializeMsgPack(doc, payload, length);

            if (error) {
                Serial.println(F("Error deserializing message"));
                break;
            }

            V2Notify::handleV2Message(doc);
            break;
        }
        case WStype_ERROR:
        case WStype_FRAGMENT_TEXT_START:
        case WStype_FRAGMENT_BIN_START:
        case WStype_FRAGMENT:
        case WStype_PING:
        case WStype_PONG:
        case WStype_FRAGMENT_FIN:
            break;
        }
    }

    void handleV2Message(JsonDocument doc)
    {
        if (doc["blockheight"].is<uint>())
        {
            uint newBlockHeight = doc["blockheight"].as<uint>();

            if (newBlockHeight == BlockNotify::getInstance().getBlockHeight())
            {
                return;
            }

            if (debugLogEnabled()) {
                Serial.print(F("processNewBlock "));
                Serial.println(newBlockHeight);
            }
            BlockNotify::getInstance().processNewBlock(newBlockHeight);
        }
        else if (blockFeeDecimals ? doc["blockfee2"].is<float>() : doc["blockfee"].is<float>())
        {
            float medianFee = blockFeeDecimals ? doc["blockfee2"].as<float>() : doc["blockfee"].as<float>();

            if (debugLogEnabled()) {
                Serial.print(F("processNewBlockFee "));
                Serial.println(String(medianFee, 2));
            }

            BlockNotify::getInstance().processNewBlockFee(medianFee);
        }
        else if (doc["price"].is<JsonObject>())
        {

            // Iterate through the key-value pairs of the "price" object
            for (JsonPair kv : doc["price"].as<JsonObject>())
            {
                const char *currency = kv.key().c_str();
                uint newPrice = kv.value().as<uint>();

                processNewPrice(newPrice, getCurrencyChar(currency));
            }
        }
    }

    void taskV2Notify(void *pvParameters)
    {
        for (;;)
        {
            webSocket.loop();
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }

    void restartV2Notify()
    {
        webSocket.disconnect();
        setupV2NotifyTask();
    }

    void setupV2NotifyTask()
    {
        // Never try to delete the task we're running in; that would terminate
        // this function mid-flight. Only tear it down from a different task.
        TaskHandle_t caller = xTaskGetCurrentTaskHandle();
        if (V2Notify::v2NotifyTaskHandle != NULL &&
            V2Notify::v2NotifyTaskHandle != caller)
        {
            vTaskDelete(V2Notify::v2NotifyTaskHandle);
            V2Notify::v2NotifyTaskHandle = NULL;
        }
        if (V2Notify::v2NotifyTaskHandle == NULL)
        {
            xTaskCreate(V2Notify::taskV2Notify, "v2Notify", (6 * 1024), NULL, tskIDLE_PRIORITY,
                        &V2Notify::v2NotifyTaskHandle);
        }
    }

    bool isV2NotifyConnected()
    {
        return webSocket.isConnected();
    }
}