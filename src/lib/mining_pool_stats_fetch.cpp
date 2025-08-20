#include "mining_pool_stats_fetch.hpp"

void MiningPoolStatsFetch::taskWrapper(void* pvParameters) {
    MiningPoolStatsFetch::getInstance().task();
}

void MiningPoolStatsFetch::downloadLogoTaskWrapper(void* pvParameters) {
    MiningPoolStatsFetch::getInstance().downloadLogoTask();
}

std::string MiningPoolStatsFetch::getHashRate() const {
    return hashrate;
}

int MiningPoolStatsFetch::getDailyEarnings() const {
    return dailyEarnings;
}

MiningPoolInterface* MiningPoolStatsFetch::getPool() {
    if (!currentPool) {
        std::string poolName = preferences.getString("miningPoolName", DEFAULT_MINING_POOL_NAME).c_str();
        currentPool = PoolFactory::createPool(poolName);
    }
    return currentPool.get();
}

const MiningPoolInterface* MiningPoolStatsFetch::getPool() const {
    return currentPool.get();
}

LogoData MiningPoolStatsFetch::getLogo() const {
    if (const auto* pool = getPool()) {
        return pool->getLogo();
    }
    return LogoData{};
}

void MiningPoolStatsFetch::task() {
    std::string poolName = preferences.getString("miningPoolName", DEFAULT_MINING_POOL_NAME).c_str();
    auto* poolInterface = getPool();
    if (!poolInterface) return;
    
    std::string poolUser = preferences.getString("miningPoolUser", DEFAULT_MINING_POOL_USER).c_str();

    // Main stats fetching loop
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        HTTPClient http;
        http.setUserAgent(USER_AGENT);
        
        poolInterface->setPoolUser(poolUser);
        std::string apiUrl = poolInterface->getApiUrl();
        http.begin(apiUrl.c_str());
        if (debugLogEnabled()) {
            Serial.printf("Fetching mining pool stats from %s\r\n", apiUrl.c_str());
        }
        poolInterface->prepareRequest(http);
        int httpCode = http.GET();
        if (httpCode == 200) {
            String payload = http.getString();
            JsonDocument doc;
            deserializeJson(doc, payload);

            if (debugLogEnabled()) {
                Serial.printf("Mining pool stats response: %s\r\n", payload.c_str());
            }

            PoolStats stats = poolInterface->parseResponse(doc);
            hashrate = stats.hashrate;

            if (debugLogEnabled()) {
                Serial.printf("Mining pool stats parsed hashrate: %s\r\n", stats.hashrate.c_str());
            }

            dailyEarnings = stats.dailyEarnings ? *stats.dailyEarnings : 0;

            if (workQueue != nullptr && (ScreenHandler::getCurrentScreen() == SCREEN_MINING_POOL_STATS_HASHRATE || 
                ScreenHandler::getCurrentScreen() == SCREEN_MINING_POOL_STATS_EARNINGS)) {
                WorkItem priceUpdate = {TASK_MINING_POOL_STATS_UPDATE, 0};
                xQueueSend(workQueue, &priceUpdate, portMAX_DELAY);
            }
        } else {
            Serial.print(F("Error retrieving mining pool data. HTTP status code: "));
            Serial.println(httpCode);
        }
    }
}

void MiningPoolStatsFetch::downloadLogoTask() {
    std::string poolName = preferences.getString("miningPoolName", DEFAULT_MINING_POOL_NAME).c_str();
    auto* poolInterface = getPool();
    if (!poolInterface) return;
    
    PoolFactory::downloadPoolLogo(poolName, poolInterface);

    // If we're on the mining pool stats screen, trigger a display update
    if (ScreenHandler::getCurrentScreen() == SCREEN_MINING_POOL_STATS_HASHRATE) {
        WorkItem priceUpdate = {TASK_MINING_POOL_STATS_UPDATE, 0};
        xQueueSend(workQueue, &priceUpdate, portMAX_DELAY);
    }

    xTaskNotifyGive(taskHandle);
    vTaskDelete(NULL);
}

void MiningPoolStatsFetch::setup() {
    xTaskCreate(downloadLogoTaskWrapper, 
                "logoDownload", 
                (6 * 1024),  
                NULL, 
                tskIDLE_PRIORITY,
                NULL);

    xTaskCreate(taskWrapper, 
                "miningPoolStatsFetch", 
                (6 * 1024), 
                NULL, 
                tskIDLE_PRIORITY,
                &taskHandle);
}
