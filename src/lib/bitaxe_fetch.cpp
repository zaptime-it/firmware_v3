#include "bitaxe_fetch.hpp"

void BitaxeFetch::taskWrapper(void* pvParameters) {
    BitaxeFetch::getInstance().task();
}

uint64_t BitaxeFetch::getHashRate() const {
    return hashrate;
}

uint64_t BitaxeFetch::getBestDiff() const {
    return bestDiff;
}

void BitaxeFetch::task() {
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        HTTPClient http;
        http.setUserAgent(USER_AGENT);
        String bitaxeApiUrl = "http://" + preferences.getString("bitaxeHostname", DEFAULT_BITAXE_HOSTNAME) + "/api/system/info";
        http.begin(bitaxeApiUrl.c_str());

        int httpCode = http.GET();

        if (httpCode == 200) {
            String payload = http.getString();
            JsonDocument doc;
            deserializeJson(doc, payload);
            
            // Convert GH/s to H/s (multiply by 10^9)
            float hashRateGH = doc["hashRate"].as<float>();
            hashrate = static_cast<uint64_t>(std::round(hashRateGH * std::pow(10, getHashrateMultiplier('G'))));
            
            // Parse difficulty string and convert to uint64_t
            std::string diffStr = doc["bestDiff"].as<std::string>();
            char diffUnit = diffStr[diffStr.length() - 1];
            if (std::isalpha(diffUnit)) {
                float diffValue = std::stof(diffStr.substr(0, diffStr.length() - 1));
                bestDiff = static_cast<uint64_t>(std::round(diffValue * std::pow(10, getDifficultyMultiplier(diffUnit))));
            } else {
                bestDiff = std::stoull(diffStr);
            }

            if (workQueue != nullptr && (ScreenHandler::getCurrentScreen() == SCREEN_BITAXE_HASHRATE || ScreenHandler::getCurrentScreen() == SCREEN_BITAXE_BESTDIFF)) {
                WorkItem priceUpdate = {TASK_BITAXE_UPDATE, 0};
                xQueueSend(workQueue, &priceUpdate, portMAX_DELAY);
            }
        } else {
            Serial.print(F("Error retrieving Bitaxe data. HTTP status code: "));
            Serial.println(httpCode);
            Serial.println(bitaxeApiUrl);
        }
    }
}

void BitaxeFetch::setup() {
    xTaskCreate(taskWrapper, "bitaxeFetch", (3 * 1024), NULL, tskIDLE_PRIORITY, &taskHandle);
    xTaskNotifyGive(taskHandle);
}