#include "mining_pool_stats_fetch.hpp"

TaskHandle_t miningPoolStatsFetchTaskHandle;

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
        String miningPoolStatsApiUrl;

        std::string httpHeaderKey = "";
        std::string httpHeaderValue;
        if (preferences.getString("miningPoolName", DEFAULT_MINING_POOL_NAME) == MINING_POOL_NAME_OCEAN) {
            miningPoolStatsApiUrl = "https://api.ocean.xyz/v1/statsnap/" + preferences.getString("miningPoolUser", DEFAULT_MINING_POOL_USER);
        }
        else if (preferences.getString("miningPoolName", DEFAULT_MINING_POOL_NAME) == MINING_POOL_NAME_BRAIINS) {
            miningPoolStatsApiUrl = "https://pool.braiins.com/accounts/profile/json/btc/";
            httpHeaderKey = "Pool-Auth-Token";
            httpHeaderValue = preferences.getString("miningPoolUser", DEFAULT_MINING_POOL_USER).c_str();
        }
        else
        {
            Serial.println("Unknown mining pool: \"" + preferences.getString("miningPoolName", DEFAULT_MINING_POOL_NAME) + "\"");
            continue;
        }

        http.begin(miningPoolStatsApiUrl.c_str());

        if (httpHeaderKey.length() > 0) {
            http.addHeader(httpHeaderKey.c_str(), httpHeaderValue.c_str());
        }

        int httpCode = http.GET();
 
        if (httpCode == 200)
        {
            String payload = http.getString();
            JsonDocument doc;
            deserializeJson(doc, payload);

            Serial.println(doc.as<String>());

            if (preferences.getString("miningPoolName", DEFAULT_MINING_POOL_NAME) == MINING_POOL_NAME_OCEAN) {
                miningPoolStatsHashrate = doc["result"]["hashrate_300s"].as<std::string>();
                miningPoolStatsDailyEarnings = int(doc["result"]["estimated_earn_next_block"].as<float>() * 100000000);
            }
            else if (preferences.getString("miningPoolName", DEFAULT_MINING_POOL_NAME) == MINING_POOL_NAME_BRAIINS) {
                // Reports hashrate in specific hashrate units (e.g. Gh/s); we want raw total hashes per second.
                std::string hashrateUnit = doc["btc"]["hash_rate_unit"].as<std::string>();
                int multiplier = 0;
                if (hashrateUnit == "Zh/s") {
                    multiplier = 21;
                } else if (hashrateUnit == "Eh/s") {
                    multiplier = 18;
                } else if (hashrateUnit == "Ph/s") {
                    multiplier = 15;
                } else if (hashrateUnit == "Th/s") {
                    multiplier = 12;
                } else if (hashrateUnit == "Gh/s") {
                    multiplier = 9;
                } else if (hashrateUnit == "Mh/s") {
                    multiplier = 6;
                } else if (hashrateUnit == "Kh/s") {
                    multiplier = 3;
                }

                // Add zeroes to pad to a full h/s output
                miningPoolStatsHashrate = std::to_string(static_cast<int>(std::round(doc["btc"]["hash_rate_5m"].as<float>()))) + std::string(multiplier, '0');

                miningPoolStatsDailyEarnings = int(doc["btc"]["today_reward"].as<float>() * 100000000);
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
            Serial.println(miningPoolStatsApiUrl);
        }
    }
}

void setupMiningPoolStatsFetchTask()
{
    xTaskCreate(taskMiningPoolStatsFetch, "miningPoolStatsFetch", (6 * 1024), NULL, tskIDLE_PRIORITY,
                &miningPoolStatsFetchTaskHandle);

    xTaskNotifyGive(miningPoolStatsFetchTaskHandle);
}