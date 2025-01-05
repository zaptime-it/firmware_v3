#include "bitaxe_fetch.hpp"

TaskHandle_t bitaxeFetchTaskHandle;

uint64_t bitaxeHashrate;
uint64_t bitaxeBestDiff;

uint64_t getBitAxeHashRate()
{
    return bitaxeHashrate;
}

uint64_t getBitaxeBestDiff()
{
    return bitaxeBestDiff;
}

void taskBitaxeFetch(void *pvParameters)
{
    for (;;)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        HTTPClient http;
        http.setUserAgent(USER_AGENT);
        String bitaxeApiUrl = "http://" + preferences.getString("bitaxeHostname", DEFAULT_BITAXE_HOSTNAME) + "/api/system/info";
        http.begin(bitaxeApiUrl.c_str());

        int httpCode = http.GET();

        if (httpCode == 200)
        {
            String payload = http.getString();
            JsonDocument doc;
            deserializeJson(doc, payload);
            
            // Convert GH/s to H/s (multiply by 10^9)
            float hashRateGH = doc["hashRate"].as<float>();
            bitaxeHashrate = static_cast<uint64_t>(std::round(hashRateGH * std::pow(10, getHashrateMultiplier('G'))));
            
            // Parse difficulty string and convert to uint64_t
            std::string diffStr = doc["bestDiff"].as<std::string>();
            char diffUnit = diffStr[diffStr.length() - 1];
            if (std::isalpha(diffUnit)) {
                float diffValue = std::stof(diffStr.substr(0, diffStr.length() - 1));
                bitaxeBestDiff = static_cast<uint64_t>(std::round(diffValue * std::pow(10, getDifficultyMultiplier(diffUnit))));
            } else {
                bitaxeBestDiff = std::stoull(diffStr);
            }

            if (workQueue != nullptr && (ScreenHandler::getCurrentScreen() == SCREEN_BITAXE_HASHRATE || ScreenHandler::getCurrentScreen() == SCREEN_BITAXE_BESTDIFF))
            {
                WorkItem priceUpdate = {TASK_BITAXE_UPDATE, 0};
                xQueueSend(workQueue, &priceUpdate, portMAX_DELAY);
            }
        }
        else
        {
            Serial.print(
                F("Error retrieving BitAxe data. HTTP status code: "));
            Serial.println(httpCode);
            Serial.println(bitaxeApiUrl);
        }
    }
}

void setupBitaxeFetchTask()
{
    xTaskCreate(taskBitaxeFetch, "bitaxeFetch", (3 * 1024), NULL, tskIDLE_PRIORITY,
                &bitaxeFetchTaskHandle);

    xTaskNotifyGive(bitaxeFetchTaskHandle);
}