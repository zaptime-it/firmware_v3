#include "block_notify.hpp"

// Initialize static members
esp_websocket_client_handle_t BlockNotify::wsClient = nullptr;
uint32_t BlockNotify::currentBlockHeight = 878000;
uint16_t BlockNotify::blockMedianFee = 1;
bool BlockNotify::notifyInit = false;
unsigned long int BlockNotify::lastBlockUpdate = 0;
TaskHandle_t BlockNotify::taskHandle = nullptr;

const char* BlockNotify::mempoolWsCert = R"EOF(
-----BEGIN CERTIFICATE-----
MIIF3jCCA8agAwIBAgIQAf1tMPyjylGoG7xkDjUDLTANBgkqhkiG9w0BAQwFADCB
iDELMAkGA1UEBhMCVVMxEzARBgNVBAgTCk5ldyBKZXJzZXkxFDASBgNVBAcTC0pl
cnNleSBDaXR5MR4wHAYDVQQKExVUaGUgVVNFUlRSVVNUIE5ldHdvcmsxLjAsBgNV
BAMTJVVTRVJUcnVzdCBSU0EgQ2VydGlmaWNhdGlvbiBBdXRob3JpdHkwHhcNMTAw
MjAxMDAwMDAwWhcNMzgwMTE4MjM1OTU5WjCBiDELMAkGA1UEBhMCVVMxEzARBgNV
BAgTCk5ldyBKZXJzZXkxFDASBgNVBAcTC0plcnNleSBDaXR5MR4wHAYDVQQKExVU
aGUgVVNFUlRSVVNUIE5ldHdvcmsxLjAsBgNVBAMTJVVTRVJUcnVzdCBSU0EgQ2Vy
dGlmaWNhdGlvbiBBdXRob3JpdHkwggIiMA0GCSqGSIb3DQEBAQUAA4ICDwAwggIK
AoICAQCAEmUXNg7D2wiz0KxXDXbtzSfTTK1Qg2HiqiBNCS1kCdzOiZ/MPans9s/B
3PHTsdZ7NygRK0faOca8Ohm0X6a9fZ2jY0K2dvKpOyuR+OJv0OwWIJAJPuLodMkY
tJHUYmTbf6MG8YgYapAiPLz+E/CHFHv25B+O1ORRxhFnRghRy4YUVD+8M/5+bJz/
Fp0YvVGONaanZshyZ9shZrHUm3gDwFA66Mzw3LyeTP6vBZY1H1dat//O+T23LLb2
VN3I5xI6Ta5MirdcmrS3ID3KfyI0rn47aGYBROcBTkZTmzNg95S+UzeQc0PzMsNT
79uq/nROacdrjGCT3sTHDN/hMq7MkztReJVni+49Vv4M0GkPGw/zJSZrM233bkf6
c0Plfg6lZrEpfDKEY1WJxA3Bk1QwGROs0303p+tdOmw1XNtB1xLaqUkL39iAigmT
Yo61Zs8liM2EuLE/pDkP2QKe6xJMlXzzawWpXhaDzLhn4ugTncxbgtNMs+1b/97l
c6wjOy0AvzVVdAlJ2ElYGn+SNuZRkg7zJn0cTRe8yexDJtC/QV9AqURE9JnnV4ee
UB9XVKg+/XRjL7FQZQnmWEIuQxpMtPAlR1n6BB6T1CZGSlCBst6+eLf8ZxXhyVeE
Hg9j1uliutZfVS7qXMYoCAQlObgOK6nyTJccBz8NUvXt7y+CDwIDAQABo0IwQDAd
BgNVHQ4EFgQUU3m/WqorSs9UgOHYm8Cd8rIDZsswDgYDVR0PAQH/BAQDAgEGMA8G
A1UdEwEB/wQFMAMBAf8wDQYJKoZIhvcNAQEMBQADggIBAFzUfA3P9wF9QZllDHPF
Up/L+M+ZBn8b2kMVn54CVVeWFPFSPCeHlCjtHzoBN6J2/FNQwISbxmtOuowhT6KO
VWKR82kV2LyI48SqC/3vqOlLVSoGIG1VeCkZ7l8wXEskEVX/JJpuXior7gtNn3/3
ATiUFJVDBwn7YKnuHKsSjKCaXqeYalltiz8I+8jRRa8YFWSQEg9zKC7F4iRO/Fjs
8PRF/iKz6y+O0tlFYQXBl2+odnKPi4w2r78NBc5xjeambx9spnFixdjQg3IM8WcR
iQycE0xyNN+81XHfqnHd4blsjDwSXWXavVcStkNr/+XeTWYRUc+ZruwXtuhxkYze
Sf7dNXGiFSeUHM9h4ya7b6NnJSFd5t0dCy5oGzuCr+yDZ4XUmFF0sbmZgIn/f3gZ
XHlKYC6SQK5MNyosycdiyA5d9zZbyuAlJQG03RoHnHcAP9Dc1ew91Pq7P8yF1m9/
qS3fuQL39ZeatTXaw2ewh0qpKJ4jjv9cJ2vhsE/zB+4ALtRZh8tSQZXq9EfX7mRB
VXyNWQKV3WKdwrnuWih0hKWbt5DHDAff9Yk2dDLWKMGwsAvgnEzDHNb842m1R0aB
L6KCq9NjRHDEjf8tM7qtj3u1cIiuPhnPQCjY/MiQu12ZIvVS5ljFH4gxQ+6IHdfG
jjxDah2nGN59PRbxYvnKkKj9
-----END CERTIFICATE-----
)EOF";

void BlockNotify::onWebsocketEvent(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
    BlockNotify& instance = BlockNotify::getInstance();

    switch (event_id) {
        case WEBSOCKET_EVENT_CONNECTED:
        {
            notifyInit = true;
            Serial.print(F("Connected to "));
            Serial.println(preferences.getString("mempoolInstance", DEFAULT_MEMPOOL_INSTANCE));

            JsonDocument doc;
            doc["action"] = "want";
            JsonArray dataArray = doc.createNestedArray("data");
            dataArray.add("blocks");
            dataArray.add("mempool-blocks");
            
            String sub;
            serializeJson(doc, sub);
            esp_websocket_client_send_text(wsClient, sub.c_str(), sub.length(), portMAX_DELAY);
            break;
        }
        case WEBSOCKET_EVENT_DATA:
            instance.onWebsocketMessage(data);
            break;

        case WEBSOCKET_EVENT_DISCONNECTED:
            Serial.println(F("Mempool.space WS Connection Closed"));
            break;

        case WEBSOCKET_EVENT_ERROR:
            Serial.println(F("Mempool.space WS Connection Error"));
            break;
    }
}

void BlockNotify::onWebsocketMessage(esp_websocket_event_data_t *data) {
    JsonDocument doc;
    JsonDocument filter;
    filter["block"]["height"] = true;
    filter["mempool-blocks"][0]["medianFee"] = true;

    deserializeJson(doc, (char*)data->data_ptr, DeserializationOption::Filter(filter));

    if (doc["block"].is<JsonObject>()) {
        JsonObject block = doc["block"];
        if (block["height"].as<uint>() != currentBlockHeight) {
            processNewBlock(block["height"].as<uint>());
        }
    }
    else if (doc["mempool-blocks"].is<JsonArray>()) {
        JsonArray blockInfo = doc["mempool-blocks"].as<JsonArray>();
        uint medianFee = (uint)round(blockInfo[0]["medianFee"].as<double>());
        processNewBlockFee(medianFee);
    }
}

void BlockNotify::setup() {
    IPAddress result;
    int dnsErr = -1;
    String mempoolInstance = preferences.getString("mempoolInstance", DEFAULT_MEMPOOL_INSTANCE);

    while (dnsErr != 1 && !strchr(mempoolInstance.c_str(), ':')) {
        dnsErr = WiFi.hostByName(mempoolInstance.c_str(), result);

        if (dnsErr != 1) {
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

    if (currentBlockHeight != -1) {
        lastBlockUpdate = esp_timer_get_time() / 1000000;
    }

    if (workQueue != nullptr) {
        WorkItem blockUpdate = {TASK_BLOCK_UPDATE, 0};
        xQueueSend(workQueue, &blockUpdate, portMAX_DELAY);
    }

    const bool useSSL = preferences.getBool("mempoolSecure", DEFAULT_MEMPOOL_SECURE);
    const String protocol = useSSL ? "wss" : "ws";
    String wsUri = protocol + "://" + mempoolInstance + "/api/v1/ws";

    esp_websocket_client_config_t config = {
        .task_stack = (6*1024),
        .user_agent = USER_AGENT
    };

    if (useSSL) {
        config.cert_pem = mempoolWsCert;
    }

    config.uri = wsUri.c_str();

    Serial.printf("Connecting to %s\r\n", mempoolInstance.c_str());

    wsClient = esp_websocket_client_init(&config);
    esp_websocket_register_events(wsClient, WEBSOCKET_EVENT_ANY, onWebsocketEvent, wsClient);
    esp_websocket_client_start(wsClient);
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
    if (wsClient == NULL)
        return false;
    return esp_websocket_client_is_connected(wsClient);
}

bool BlockNotify::isInitialized() const
{
    return notifyInit;
}

void BlockNotify::stop()
{
    if (wsClient == NULL)
        return;

    esp_websocket_client_close(wsClient, portMAX_DELAY);
    esp_websocket_client_stop(wsClient);
    esp_websocket_client_destroy(wsClient);
    wsClient = NULL;
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