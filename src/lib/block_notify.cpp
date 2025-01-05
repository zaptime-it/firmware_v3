#include "block_notify.hpp"

// Initialize static members
WebSocketsClient BlockNotify::webSocket;
uint32_t BlockNotify::currentBlockHeight = 878000;
uint16_t BlockNotify::blockMedianFee = 1;
bool BlockNotify::notifyInit = false;
unsigned long int BlockNotify::lastBlockUpdate = 0;
TaskHandle_t BlockNotify::taskHandle = nullptr;

void BlockNotify::taskNotify(void* pvParameters)
{
    for (;;)
    {
        webSocket.loop();
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void BlockNotify::setupTask()
{
    xTaskCreate(taskNotify, "blockNotify", (6 * 1024), NULL, tskIDLE_PRIORITY,
                &taskHandle);
}

void BlockNotify::onWebsocketEvent(WStype_t type, uint8_t* payload, size_t length) {
    switch(type) {
        case WStype_DISCONNECTED: {
            Serial.println(F("Mempool.space WS Connection Closed"));
            break;
        }
        case WStype_CONNECTED: {
            notifyInit = true;
            Serial.print(F("Connected to "));
            Serial.println(preferences.getString("mempoolInstance", DEFAULT_MEMPOOL_INSTANCE));

            JsonDocument doc;
            doc["action"] = "want";
            JsonArray data = doc.createNestedArray("data");
            data.add("blocks");
            data.add("mempool-blocks");
            
            String sub;
            serializeJson(doc, sub);
            Serial.println(sub);
            webSocket.sendTXT(sub.c_str());
            break;
        }
        case WStype_TEXT: {
            JsonDocument doc;
            JsonDocument filter;
            filter["block"]["height"] = true;
            filter["mempool-blocks"][0]["medianFee"] = true;

            deserializeJson(doc, (char*)payload, DeserializationOption::Filter(filter));

            if (debugLogEnabled()) {
                Serial.println(doc.as<String>());
            }

            if (doc["block"].is<JsonObject>())
            {
                JsonObject block = doc["block"];
                if (block["height"].as<uint>() != currentBlockHeight) {
                    BlockNotify::getInstance().processNewBlock(block["height"].as<uint>());
                }
            }
            else if (doc["mempool-blocks"].is<JsonArray>())
            {
                JsonArray blockInfo = doc["mempool-blocks"].as<JsonArray>();
                uint medianFee = (uint)round(blockInfo[0]["medianFee"].as<double>());
                BlockNotify::getInstance().processNewBlockFee(medianFee);
            }
            break;
        }
        case WStype_BIN:
        case WStype_ERROR:
        case WStype_FRAGMENT_TEXT_START:
        case WStype_FRAGMENT_BIN_START:
        case WStype_FRAGMENT:
        case WStype_PING:
        case WStype_PONG:
        case WStype_FRAGMENT_FIN: {
            break;
        }
    }
}

void BlockNotify::setup()
{
    IPAddress result;
    int dnsErr = -1;
    String mempoolInstance = preferences.getString("mempoolInstance", DEFAULT_MEMPOOL_INSTANCE);

    while (dnsErr != 1 && !strchr(mempoolInstance.c_str(), ':'))
    {
        dnsErr = WiFi.hostByName(mempoolInstance.c_str(), result);

        if (dnsErr != 1)
        {
            Serial.print(mempoolInstance);
            Serial.println(F("mempool DNS could not be resolved"));
            WiFi.reconnect();
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    // Get current block height through regular API
    int blockFetch = fetchLatestBlock();

    if (blockFetch > currentBlockHeight)
        currentBlockHeight = blockFetch;

    if (currentBlockHeight != -1)
    {
        lastBlockUpdate = esp_timer_get_time() / 1000000;
    }

    if (workQueue != nullptr)
    {
        WorkItem blockUpdate = {TASK_BLOCK_UPDATE, 0};
        xQueueSend(workQueue, &blockUpdate, portMAX_DELAY);
    }

    const bool useSSL = preferences.getBool("mempoolSecure", DEFAULT_MEMPOOL_SECURE);
    const int port = useSSL ? 443 : 80;

    if (useSSL) {
        webSocket.beginSSL(mempoolInstance.c_str(), port, "/api/v1/ws");
//          webSocket.beginSSL("ws.btclock.dev", port, "/api/v1/ws");

    } else {
        webSocket.begin(mempoolInstance.c_str(), port, "/api/v1/ws");
    }

    webSocket.onEvent(onWebsocketEvent);
    webSocket.setReconnectInterval(5000);
    webSocket.enableHeartbeat(15000, 3000, 2);

    setupTask();
}

void BlockNotify::processNewBlock(uint32_t newBlockHeight) {
    if (newBlockHeight <= currentBlockHeight)
    {
        return;
    }

    currentBlockHeight = newBlockHeight;
    lastBlockUpdate = esp_timer_get_time() / 1000000;

    if (workQueue != nullptr)
    {
        WorkItem blockUpdate = {TASK_BLOCK_UPDATE, 0};
        xQueueSend(workQueue, &blockUpdate, portMAX_DELAY);
    }

    if (ScreenHandler::getCurrentScreen() != SCREEN_BLOCK_HEIGHT &&
        preferences.getBool("stealFocus", DEFAULT_STEAL_FOCUS))
    {
        uint64_t timerPeriod = 0;
        if (isTimerActive())
        {
            timerPeriod = getTimerSeconds();
            esp_timer_stop(screenRotateTimer);
        }
        ScreenHandler::setCurrentScreen(SCREEN_BLOCK_HEIGHT);
        if (timerPeriod > 0)
        {
            esp_timer_start_periodic(screenRotateTimer,
                                   timerPeriod * usPerSecond);
        }
        vTaskDelay(pdMS_TO_TICKS(315*NUM_SCREENS)); // Extra delay because of screen switching
    }

    if (preferences.getBool("ledFlashOnUpd", DEFAULT_LED_FLASH_ON_UPD))
    {
        vTaskDelay(pdMS_TO_TICKS(250)); // Wait until screens are updated
        getLedHandler().queueEffect(LED_FLASH_BLOCK_NOTIFY);
    }
}

void BlockNotify::processNewBlockFee(uint16_t newBlockFee) {
    if (blockMedianFee == newBlockFee)
    {
        return;
    }

    blockMedianFee = newBlockFee;

    if (workQueue != nullptr)
    {
        WorkItem blockUpdate = {TASK_FEE_UPDATE, 0};
        xQueueSend(workQueue, &blockUpdate, portMAX_DELAY);
    }
}

uint32_t BlockNotify::getBlockHeight() const { 
    return currentBlockHeight; 
}

void BlockNotify::setBlockHeight(uint32_t newBlockHeight)
{
    currentBlockHeight = newBlockHeight;
}

uint16_t BlockNotify::getBlockMedianFee() const { 
    return blockMedianFee; 
}

void BlockNotify::setBlockMedianFee(uint16_t newBlockMedianFee)
{
    blockMedianFee = newBlockMedianFee;
}

bool BlockNotify::isConnected() const
{
    return webSocket.isConnected();
}

bool BlockNotify::isInitialized() const
{
    return notifyInit;
}

void BlockNotify::stop()
{
    webSocket.disconnect();
    if (taskHandle != NULL) {
        vTaskDelete(taskHandle);
        taskHandle = NULL;
    }
}

void BlockNotify::restart()
{
    stop();
    setup();
}

int BlockNotify::fetchLatestBlock() {
    try {
        String mempoolInstance = preferences.getString("mempoolInstance", DEFAULT_MEMPOOL_INSTANCE);
        const String protocol = preferences.getBool("mempoolSecure", DEFAULT_MEMPOOL_SECURE) ? "https" : "http";
        String url = protocol + "://" + mempoolInstance + "/api/blocks/tip/height";

        HTTPClient* http = HttpHelper::begin(url);
        Serial.println("Fetching block height from " + url);
        int httpCode = http->GET();

        if (httpCode > 0 && httpCode == HTTP_CODE_OK) {
            String blockHeightStr = http->getString();
            HttpHelper::end(http);
            return blockHeightStr.toInt();
        }
        HttpHelper::end(http);
        Serial.println("HTTP code" + String(httpCode));
    } catch (...) {
        Serial.println(F("An exception occurred while trying to get the latest block"));
    }
    return 2203; // B-T-C
}

uint BlockNotify::getLastBlockUpdate() const
{
    return lastBlockUpdate;
}

void BlockNotify::setLastBlockUpdate(uint lastUpdate)
{
    lastBlockUpdate = lastUpdate;
}