#include "block_notify.hpp"

#include <mutex>
#include <utility>

#include "lib/system/tls_gate.hpp"

// Initialize static members
WebSocketsClient BlockNotify::wsClient;
std::atomic<uint32_t> BlockNotify::currentBlockHeight{INITIAL_BLOCK_HEIGHT};
std::atomic<float> BlockNotify::blockMedianFee{1.0f};
std::atomic<bool> BlockNotify::notifyInit{false};
std::atomic<bool> BlockNotify::wsConnected{false};
std::atomic<unsigned long> BlockNotify::lastBlockUpdate{0};
std::atomic<bool> BlockNotify::shouldStop{false};
TaskHandle_t BlockNotify::taskHandle = nullptr;

namespace {

// Split "host:port" into its components. If no port is present the supplied
// default is used. Leading/trailing whitespace is preserved; callers pass
// already-trimmed preference values.
std::pair<String, uint16_t> splitHostPort(const String &endpoint, uint16_t defaultPort) {
    int colon = endpoint.indexOf(':');
    if (colon < 0) {
        return {endpoint, defaultPort};
    }
    String host = endpoint.substring(0, colon);
    String portStr = endpoint.substring(colon + 1);
    long port = portStr.toInt();
    if (port <= 0 || port > 65535) {
        return {host, defaultPort};
    }
    return {host, static_cast<uint16_t>(port)};
}

}  // namespace

void BlockNotify::onWebsocketEvent(WStype_t type, uint8_t *payload, size_t length) {
    BlockNotify& instance = BlockNotify::getInstance();

    switch (type) {
        case WStype_CONNECTED: {
            notifyInit.store(true, std::memory_order_relaxed);
            wsConnected.store(true, std::memory_order_relaxed);
            Serial.print(F("Connected to "));
            Serial.println(preferences.getString("mempoolInstance", DEFAULT_MEMPOOL_INSTANCE));

            JsonDocument doc;
            doc["action"] = "want";
            JsonArray dataArray = doc.createNestedArray("data");
            dataArray.add("blocks");
            dataArray.add("mempool-blocks");

            String sub;
            serializeJson(doc, sub);
            wsClient.sendTXT(sub);
            break;
        }
        case WStype_TEXT:
            instance.onWebsocketMessage(payload, length);
            break;

        case WStype_DISCONNECTED:
            notifyInit.store(false, std::memory_order_relaxed);
            wsConnected.store(false, std::memory_order_relaxed);
            Serial.println(F("Mempool.space WS Connection Closed"));
            break;

        case WStype_ERROR:
            Serial.println(F("Mempool.space WS Connection Error"));
            break;

        case WStype_BIN:
        case WStype_FRAGMENT_TEXT_START:
        case WStype_FRAGMENT_BIN_START:
        case WStype_FRAGMENT:
        case WStype_FRAGMENT_FIN:
        case WStype_PING:
        case WStype_PONG:
            break;
    }
}

void BlockNotify::onWebsocketMessage(uint8_t *payload, size_t length) {
    JsonDocument doc;
    JsonDocument filter;
    filter["block"]["height"] = true;
    filter["mempool-blocks"][0]["medianFee"] = true;

    if (payload == nullptr || length == 0) {
        return;
    }

    DeserializationError err = deserializeJson(doc, (const char*)payload, length,
                                               DeserializationOption::Filter(filter));
    if (err) {
        Serial.printf("BlockNotify bad JSON: %s\r\n", err.c_str());
        return;
    }

    if (doc["block"].is<JsonObject>()) {
        JsonObject block = doc["block"];
        if (block["height"].as<uint>() != currentBlockHeight.load(std::memory_order_relaxed)) {
            processNewBlock(block["height"].as<uint>());
        }
    }
    else if (doc["mempool-blocks"].is<JsonArray>()) {
        JsonArray blockInfo = doc["mempool-blocks"].as<JsonArray>();
        // Previously indexed [0] without checking the array actually had
        // entries, which returned a null JsonVariant cast to NaN; round(NaN)
        // then yielded an undefined integer.
        if (blockInfo.size() == 0 || !blockInfo[0]["medianFee"].is<double>()) {
            return;
        }
        uint medianFee = (uint)round(blockInfo[0]["medianFee"].as<double>());
        processNewBlockFee(medianFee);
    }
}

void BlockNotify::setup() {
    String mempoolInstance = preferences.getString("mempoolInstance", DEFAULT_MEMPOOL_INSTANCE);
    const bool useSSL = preferences.getBool("mempoolSecure", DEFAULT_MEMPOOL_SECURE);

    // Resolve DNS up-front only when the user did not pin an IP:port.
    if (mempoolInstance.indexOf(':') < 0) {
        IPAddress result;
        int dnsErr = -1;
        while (dnsErr != 1) {
            dnsErr = WiFi.hostByName(mempoolInstance.c_str(), result);
            if (dnsErr != 1) {
                Serial.print(mempoolInstance);
                Serial.println(F(" mempool DNS could not be resolved"));
                WiFi.reconnect();
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
        }
    }

    // Get current block height through regular API
    int blockFetch = fetchLatestBlock();
    if (blockFetch > static_cast<int>(currentBlockHeight.load(std::memory_order_relaxed)))
        currentBlockHeight.store(static_cast<uint32_t>(blockFetch), std::memory_order_relaxed);
    if (currentBlockHeight.load(std::memory_order_relaxed) != 0) {
        lastBlockUpdate.store(esp_timer_get_time() / 1000000, std::memory_order_relaxed);
    }
    if (workQueue != nullptr) {
        WorkItem blockUpdate = {TASK_BLOCK_UPDATE, 0};
        xQueueSend(workQueue, &blockUpdate, portMAX_DELAY);
    }

    auto hostPort = splitHostPort(mempoolInstance, useSSL ? 443 : 80);
    Serial.printf("Connecting to %s:%u\r\n", hostPort.first.c_str(), hostPort.second);

    if (useSSL) {
        wsClient.beginSSL(hostPort.first, hostPort.second, "/api/v1/ws");
    } else {
        wsClient.begin(hostPort.first, hostPort.second, "/api/v1/ws");
    }
    wsClient.onEvent(BlockNotify::onWebsocketEvent);
    wsClient.setReconnectInterval(5000);
    wsClient.enableHeartbeat(15000, 3000, 2);

    // Give the pump task a single-source-of-truth lifetime.
    TaskHandle_t caller = xTaskGetCurrentTaskHandle();
    if (taskHandle != nullptr && taskHandle != caller) {
        vTaskDelete(taskHandle);
        taskHandle = nullptr;
    }
    if (taskHandle == nullptr) {
        xTaskCreate(BlockNotify::taskBlockNotify, "blockNotify", (6 * 1024), nullptr,
                    tskIDLE_PRIORITY, &taskHandle);
    }
}

void BlockNotify::taskBlockNotify(void *pvParameters) {
    for (;;) {
        if (shouldStop.load(std::memory_order_relaxed)) {
            shouldStop.store(false, std::memory_order_relaxed);
            taskHandle = nullptr;
            vTaskDelete(nullptr);
            return;
        }
        // When disconnected, a wsClient.loop() call runs the full TLS
        // handshake inline. Serialise that with every other in-flight
        // TLS handshake via the firmware-wide gate. Already-connected
        // iterations skip the lock entirely so steady-state event
        // handling stays contention-free.
        if (wsClient.isConnected()) {
            wsClient.loop();
        } else {
            std::lock_guard<std::mutex> lk(tls_gate::mutex());
            wsClient.loop();
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}



void BlockNotify::processNewBlock(uint32_t newBlockHeight) {
    uint32_t oldBlockHeight = currentBlockHeight.load(std::memory_order_relaxed);
    if (newBlockHeight <= oldBlockHeight)
    {
        return;
    }

    lastBlockUpdate.store(esp_timer_get_time() / 1000000, std::memory_order_relaxed);
    currentBlockHeight.store(newBlockHeight, std::memory_order_relaxed);

    if (workQueue != nullptr)
    {
        WorkItem blockUpdate = {TASK_BLOCK_UPDATE, 0};
        xQueueSend(workQueue, &blockUpdate, portMAX_DELAY);
    }

    if (newBlockHeight - oldBlockHeight > 100)
    {
        // Old block height is too far behind, update it but don't steal focus or flash LED
        preferences.putUInt("blockHeight", newBlockHeight);
        return;
    }

    if (ScreenHandler::getCurrentScreen() != SCREEN_BLOCK_HEIGHT &&
        preferences.getBool("stealFocus", DEFAULT_STEAL_FOCUS))
    {
        if (ScreenHandler::getCurrentScreen() == SCREEN_CUSTOM)
        {
            // Don't steal focus from custom screen
            return;
        }

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

void BlockNotify::processNewBlockFee(float newBlockFee) {
    if (blockMedianFee.load(std::memory_order_relaxed) == newBlockFee)
    {
        return;
    }

    blockMedianFee.store(newBlockFee, std::memory_order_relaxed);

    if (workQueue != nullptr)
    {
        WorkItem blockUpdate = {TASK_FEE_UPDATE, 0};
        xQueueSend(workQueue, &blockUpdate, portMAX_DELAY);
    }
}

uint32_t BlockNotify::getBlockHeight() const {
    return currentBlockHeight.load(std::memory_order_relaxed);
}

void BlockNotify::setBlockHeight(uint32_t newBlockHeight)
{
    currentBlockHeight.store(newBlockHeight, std::memory_order_relaxed);

    if (newBlockHeight % 100 == 0) {
        preferences.putUInt("blockHeight", newBlockHeight);
    }
}

float BlockNotify::getBlockMedianFee() const {
    return blockMedianFee.load(std::memory_order_relaxed);
}

void BlockNotify::setBlockMedianFee(float newBlockMedianFee)
{
    blockMedianFee.store(newBlockMedianFee, std::memory_order_relaxed);
}

bool BlockNotify::isConnected() const
{
    return wsConnected.load(std::memory_order_relaxed);
}

bool BlockNotify::isInitialized() const
{
    return notifyInit.load(std::memory_order_relaxed);
}

void BlockNotify::stop()
{
    notifyInit.store(false, std::memory_order_relaxed);
    wsConnected.store(false, std::memory_order_relaxed);
    wsClient.disconnect();

    TaskHandle_t caller = xTaskGetCurrentTaskHandle();
    if (taskHandle != nullptr && taskHandle != caller) {
        // Ask the pump task to exit at the top of its next loop iteration
        // instead of vTaskDelete()ing it from under a running
        // wsClient.loop() call. Wait up to ~250 ms for it to acknowledge.
        shouldStop.store(true, std::memory_order_relaxed);
        for (int i = 0; i < 25 && taskHandle != nullptr; i++) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        if (taskHandle != nullptr) {
            // Fallback if the task didn't observe the flag in time.
            vTaskDelete(taskHandle);
            taskHandle = nullptr;
        }
    }
}

void BlockNotify::restart()
{
    stop();
    setup();
}

int BlockNotify::fetchLatestBlock() {
    String mempoolInstance = preferences.getString("mempoolInstance", DEFAULT_MEMPOOL_INSTANCE);
    const String protocol = preferences.getBool("mempoolSecure", DEFAULT_MEMPOOL_SECURE) ? "https" : "http";
    String url = protocol + "://" + mempoolInstance + "/api/blocks/tip/height";

    auto http = HttpHelper::beginScoped(url);
    if (!http) {
        return 2203;
    }
    int httpCode = http->GET();
    if (httpCode == HTTP_CODE_OK) {
        String blockHeightStr = http->getString();
        return blockHeightStr.toInt();
    }
    Serial.printf("fetchLatestBlock http=%d\r\n", httpCode);
    return 2203; // B-T-C
}

uint BlockNotify::getLastBlockUpdate() const
{
    return static_cast<uint>(lastBlockUpdate.load(std::memory_order_relaxed));
}

void BlockNotify::setLastBlockUpdate(uint lastUpdate)
{
    lastBlockUpdate.store(lastUpdate, std::memory_order_relaxed);
}