#include "v2_notify.hpp"

using namespace V2Notify;

namespace V2Notify
{
    WebSocketsClient webSocket;

    TaskHandle_t v2NotifyTaskHandle;

    String currentHostname;

    void setupV2Notify()
    {
        String hostname = "ws.btclock.dev";
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
            Serial.print(F("[WSc] Disconnected!\n"));
            break;
        case WStype_CONNECTED:
        {
            Serial.print(F("[WSc] Connected to "));
            Serial.print(currentHostname);
            Serial.print(F(": "));
            Serial.println((char *)payload);

            JsonDocument response;

            response["type"] = "subscribe";
            response["eventType"] = "blockfee";
            size_t responseLength = measureMsgPack(response);
            uint8_t *buffer = new uint8_t[responseLength];
            serializeMsgPack(response, buffer, responseLength);
            webSocket.sendBIN(buffer, responseLength);
            delete[] buffer;

            buffer = new uint8_t[responseLength];

            response["type"] = "subscribe";
            response["eventType"] = "blockheight";
            responseLength = measureMsgPack(response);
            buffer = new uint8_t[responseLength];
            serializeMsgPack(response, buffer, responseLength);
            webSocket.sendBIN(buffer, responseLength);

            delete[] buffer;

            buffer = new uint8_t[responseLength];

            response["type"] = "subscribe";
            response["eventType"] = "price";

            JsonArray currenciesArray = response["currencies"].to<JsonArray>();

            for (const auto &str : getActiveCurrencies())
            {
                currenciesArray.add(str);
            }

            //            response["currencies"] = currenciesArray;
            responseLength = measureMsgPack(response);
            buffer = new uint8_t[responseLength];
            serializeMsgPack(response, buffer, responseLength);
            webSocket.sendBIN(buffer, responseLength);
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
        else if (doc["blockfee"].is<uint>())
        {
            uint medianFee = doc["blockfee"].as<uint>();

            if (debugLogEnabled()) {
                Serial.print(F("processNewBlockFee "));
                Serial.println(medianFee);
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

    void setupV2NotifyTask()
    {
        xTaskCreate(V2Notify::taskV2Notify, "v2Notify", (6 * 1024), NULL, tskIDLE_PRIORITY,
                    &V2Notify::v2NotifyTaskHandle);
    }

    bool isV2NotifyConnected()
    {
        return webSocket.isConnected();
    }
}