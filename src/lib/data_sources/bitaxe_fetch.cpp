#include "bitaxe_fetch.hpp"

#include <ArduinoJson.h>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <string>

#include "lib/system/timers.hpp"

namespace {

/** True for JSON numbers (int or float) as emitted by current AxeOS
 * `/api/system/info`. */
bool jsonVariantIsNumeric(JsonVariantConst v) {
  return v.is<double>() || v.is<float>() || v.is<signed char>() ||
         v.is<short>() || v.is<int>() || v.is<long>() || v.is<long long>() ||
         v.is<unsigned char>() || v.is<unsigned short>() ||
         v.is<unsigned int>() || v.is<unsigned long>() ||
         v.is<unsigned long long>();
}

/**
 * AxeOS historically returned `bestDiff` as a human string (optionally suffixed
 * with K/M/G/…). Newer firmware returns a raw JSON number. Accept both.
 */
bool parseBestDifficulty(JsonVariantConst v, uint64_t &out) {
  if (v.isNull())
    return false;

  if (jsonVariantIsNumeric(v)) {
    const double d = v.as<double>();
    if (d < 0)
      return false;
    if (d > static_cast<double>(std::numeric_limits<uint64_t>::max())) {
      out = std::numeric_limits<uint64_t>::max();
    } else {
      out = static_cast<uint64_t>(std::llround(d));
    }
    return true;
  }

  if (!v.is<const char *>())
    return false;

  std::string diffStr = v.as<std::string>();
  if (diffStr.empty())
    return false;

  const char diffUnit = diffStr[diffStr.length() - 1];
  if (std::isalpha(static_cast<unsigned char>(diffUnit))) {
    char *endp = nullptr;
    const float diffValue = strtof(diffStr.c_str(), &endp);
    if (endp == diffStr.c_str())
      return false;
    out = static_cast<uint64_t>(std::round(
        diffValue * std::pow(10, getDifficultyMultiplier(diffUnit))));
  } else {
    char *endp = nullptr;
    out = strtoull(diffStr.c_str(), &endp, 10);
    if (endp == diffStr.c_str())
      return false;
  }
  return true;
}

} // namespace

void BitaxeFetch::taskWrapper(void *pvParameters) {
  BitaxeFetch::getInstance().task();
}

uint64_t BitaxeFetch::getHashRate() const { return hashrate; }

uint64_t BitaxeFetch::getBestDiff() const { return bestDiff; }

void BitaxeFetch::task() {
  for (;;) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    String bitaxeApiUrl =
        "http://" +
        preferences.getString("bitaxeHostname", DEFAULT_BITAXE_HOSTNAME) +
        "/api/system/info";
    auto http = HttpHelper::beginScoped(bitaxeApiUrl);
    if (!http)
      continue;

    int httpCode = http->GET();

    if (httpCode != HTTP_CODE_OK) {
      Serial.print(F("Error retrieving Bitaxe data. HTTP status code: "));
      Serial.println(httpCode);
      Serial.println(bitaxeApiUrl);
      continue;
    }

    // Stream directly from the HTTP client + filter to the two fields
    // the display actually needs. `http->getString()` would otherwise
    // buffer the entire AxeOS `/api/system/info` response (~2 KB) as an
    // Arduino String before parsing — on Rev B with bitaxe + mining
    // pool + nostr + mempool WS + Kraken WS all active, that peak
    // allocation pushes DRAM below what mbedtls needs for its in-flight
    // TLS sessions.
    JsonDocument filter;
    filter["hashRate"] = true;
    filter["bestDiff"] = true;

    JsonDocument doc;
    DeserializationError jsonErr = deserializeJson(
        doc, *http->getStreamPtr(), DeserializationOption::Filter(filter));
    if (jsonErr) {
      Serial.printf("Bitaxe: JSON parse error: %s\r\n", jsonErr.c_str());
      continue;
    }

    JsonVariantConst hashVar = doc["hashRate"];
    JsonVariantConst diffVar = doc["bestDiff"];
    if (!jsonVariantIsNumeric(hashVar)) {
      Serial.println(F("Bitaxe: bad JSON (hashRate missing or not a number)"));
      continue;
    }

    uint64_t parsedBest = 0;
    if (!parseBestDifficulty(diffVar, parsedBest)) {
      Serial.println(F("Bitaxe: bad JSON (bestDiff missing or invalid)"));
      continue;
    }
    bestDiff = parsedBest;

    // Convert GH/s to H/s (multiply by 10^9). AxeOS reports hashRate as a JSON
    // number (often stored internally as double, so we read as double rather
    // than is<float>()).
    const double hashRateGH = hashVar.as<double>();
    hashrate = static_cast<uint64_t>(
        std::round(hashRateGH * std::pow(10, getHashrateMultiplier('G'))));

    if (workQueue != nullptr &&
        (ScreenHandler::getCurrentScreen() == SCREEN_BITAXE_HASHRATE ||
         ScreenHandler::getCurrentScreen() == SCREEN_BITAXE_BESTDIFF)) {
      WorkItem priceUpdate = {TASK_BITAXE_UPDATE, 0};
      xQueueSend(workQueue, &priceUpdate, portMAX_DELAY);
    }
  }
}

void BitaxeFetch::setup() {
  xTaskCreate(taskWrapper, "bitaxeFetch", (3 * 1024), NULL, tskIDLE_PRIORITY,
              &taskHandle);
  setBitaxeTaskHandleForIsr(taskHandle);
  xTaskNotifyGive(taskHandle);
}