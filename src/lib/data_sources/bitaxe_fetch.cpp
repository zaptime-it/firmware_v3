#include "bitaxe_fetch.hpp"

#include "lib/system/timers.hpp"

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

        String bitaxeApiUrl = "http://" + preferences.getString("bitaxeHostname", DEFAULT_BITAXE_HOSTNAME) + "/api/system/info";
        auto http = HttpHelper::beginScoped(bitaxeApiUrl);
        if (!http) continue;

        int httpCode = http->GET();

        if (httpCode != HTTP_CODE_OK) {
            Serial.print(F("Error retrieving Bitaxe data. HTTP status code: "));
            Serial.println(httpCode);
            Serial.println(bitaxeApiUrl);
            continue;
        }

        JsonDocument doc;
        DeserializationError jsonErr = deserializeJson(doc, http->getString());
        if (jsonErr ||
            !doc["hashRate"].is<float>() ||
            !doc["bestDiff"].is<const char*>()) {
            Serial.println(F("Bitaxe: bad JSON"));
            continue;
        }

        // Convert GH/s to H/s (multiply by 10^9)
        float hashRateGH = doc["hashRate"].as<float>();
        hashrate = static_cast<uint64_t>(std::round(hashRateGH * std::pow(10, getHashrateMultiplier('G'))));

        // Parse difficulty string and convert to uint64_t. Use C parsers
        // so we don't pull in the exception-based std::sto* machinery.
        std::string diffStr = doc["bestDiff"].as<std::string>();
        if (diffStr.empty()) continue;

        char diffUnit = diffStr[diffStr.length() - 1];
        if (std::isalpha(static_cast<unsigned char>(diffUnit))) {
            char* endp = nullptr;
            float diffValue = strtof(diffStr.c_str(), &endp);
            if (endp == diffStr.c_str()) continue;
            bestDiff = static_cast<uint64_t>(std::round(diffValue * std::pow(10, getDifficultyMultiplier(diffUnit))));
        } else {
            char* endp = nullptr;
            bestDiff = strtoull(diffStr.c_str(), &endp, 10);
            if (endp == diffStr.c_str()) continue;
        }

        if (workQueue != nullptr && (ScreenHandler::getCurrentScreen() == SCREEN_BITAXE_HASHRATE || ScreenHandler::getCurrentScreen() == SCREEN_BITAXE_BESTDIFF)) {
            WorkItem priceUpdate = {TASK_BITAXE_UPDATE, 0};
            xQueueSend(workQueue, &priceUpdate, portMAX_DELAY);
        }
    }
}

void BitaxeFetch::setup() {
    xTaskCreate(taskWrapper, "bitaxeFetch", (3 * 1024), NULL, tskIDLE_PRIORITY, &taskHandle);
    setBitaxeTaskHandleForIsr(taskHandle);
    xTaskNotifyGive(taskHandle);
}