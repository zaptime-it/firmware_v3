#include "mining_pool_stats_fetch.hpp"

TaskHandle_t miningPoolStatsFetchTaskHandle;

std::string miningPoolName;
std::string miningPoolStatsHashrate;
int miningPoolStatsDailyEarnings;

std::string getMiningPoolStatsHashRate()
{
    return miningPoolStatsHashrate;
}

int getMiningPoolStatsDailyEarnings()
{
    return miningPoolStatsDailyEarnings;
}

void taskMiningPoolStatsFetch(void *pvParameters)
{
    for (;;)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        HTTPClient http;
        http.setUserAgent(USER_AGENT);

        std::string poolName = preferences.getString("miningPoolName", DEFAULT_MINING_POOL_NAME).c_str();
        std::string poolUser = preferences.getString("miningPoolUser", DEFAULT_MINING_POOL_USER).c_str();

        auto poolInterface = PoolFactory::createPool(poolName);
        if (!poolInterface)
        {
            Serial.println("Unknown mining pool: \"" + String(poolName.c_str()) + "\"");
            continue;
        }

        poolInterface->setPoolUser(poolUser);
        std::string apiUrl = poolInterface->getApiUrl();
        http.begin(apiUrl.c_str());
        poolInterface->prepareRequest(http);
        int httpCode = http.GET();
        if (httpCode == 200)
        {
            String payload = http.getString();
            JsonDocument doc;
            deserializeJson(doc, payload);

            PoolStats stats = poolInterface->parseResponse(doc);

            miningPoolStatsHashrate = stats.hashrate;

            if (stats.dailyEarnings)
            {
                miningPoolStatsDailyEarnings = *stats.dailyEarnings;
            }
            else
            {
                miningPoolStatsDailyEarnings = 0; // or any other default value
            }

            if (workQueue != nullptr && (getCurrentScreen() == SCREEN_MINING_POOL_STATS_HASHRATE || getCurrentScreen() == SCREEN_MINING_POOL_STATS_EARNINGS))
            {
                WorkItem priceUpdate = {TASK_MINING_POOL_STATS_UPDATE, 0};
                xQueueSend(workQueue, &priceUpdate, portMAX_DELAY);
            }
        }
        else
        {
            Serial.print(
                F("Error retrieving mining pool data. HTTP status code: "));
            Serial.println(httpCode);
        }
    }
}

void setupMiningPoolStatsFetchTask()
{
    xTaskCreate(taskMiningPoolStatsFetch, "miningPoolStatsFetch", (6 * 1024), NULL, tskIDLE_PRIORITY,
                &miningPoolStatsFetchTaskHandle);

    xTaskNotifyGive(miningPoolStatsFetchTaskHandle);
}

std::unique_ptr<MiningPoolInterface>& getMiningPool()
{
   static std::unique_ptr<MiningPoolInterface> currentMiningPool;
    
    if (!currentMiningPool) {
        std::string poolName = preferences.getString("miningPoolName", DEFAULT_MINING_POOL_NAME).c_str();
        currentMiningPool = PoolFactory::createPool(poolName);
    }

    return currentMiningPool;
}

LogoData getMiningPoolLogo()
{
    return getMiningPool()->getLogo();
}
