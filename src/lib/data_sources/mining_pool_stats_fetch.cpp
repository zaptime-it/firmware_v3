#include "mining_pool_stats_fetch.hpp"

#include "lib/system/timers.hpp"

void MiningPoolStatsFetch::taskWrapper(void *pvParameters) {
  MiningPoolStatsFetch::getInstance().task();
}

void MiningPoolStatsFetch::downloadLogoTaskWrapper(void *pvParameters) {
  MiningPoolStatsFetch::getInstance().downloadLogoTask();
}

std::string MiningPoolStatsFetch::getHashRate() const { return hashrate; }

int64_t MiningPoolStatsFetch::getDailyEarnings() const { return dailyEarnings; }

MiningPoolInterface *MiningPoolStatsFetch::getPool() {
  if (!currentPool) {
    std::string poolName =
        preferences.getString("miningPoolName", DEFAULT_MINING_POOL_NAME)
            .c_str();
    currentPool = PoolFactory::createPool(poolName);
  }
  return currentPool.get();
}

const MiningPoolInterface *MiningPoolStatsFetch::getPool() const {
  return currentPool.get();
}

LogoData MiningPoolStatsFetch::getLogo() const {
  if (const auto *pool = getPool()) {
    return pool->getLogo();
  }
  return LogoData{};
}

void MiningPoolStatsFetch::task() {
  std::string poolName =
      preferences.getString("miningPoolName", DEFAULT_MINING_POOL_NAME).c_str();
  auto *poolInterface = getPool();
  if (!poolInterface)
    return;

  std::string poolUser =
      preferences.getString("miningPoolUser", DEFAULT_MINING_POOL_USER).c_str();

  // Main stats fetching loop
  for (;;) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    poolInterface->setPoolUser(poolUser);

    const bool useGlobal =
        preferences.getBool("poolGlobalStats", DEFAULT_POOL_GLOBAL_STATS) &&
        poolInterface->supportsGlobalStats();
    std::string apiUrl = useGlobal ? poolInterface->getGlobalStatsUrl()
                                   : poolInterface->getApiUrl();

    auto http = HttpHelper::beginScoped(apiUrl.c_str());
    if (!http)
      continue;
    poolInterface->prepareRequest(*http);

    int httpCode = http->GET();
    if (httpCode != HTTP_CODE_OK) {
      Serial.print(F("Error retrieving mining pool data. HTTP status code: "));
      Serial.println(httpCode);
      continue;
    }

    // Stream-parse straight off the HTTP client. `http->getString()`
    // would buffer the entire pool response as an Arduino String
    // before ArduinoJson even sees it — on Rev B with the full
    // THIRD_PARTY set of WS sessions (mempool + Kraken + Nostr) the
    // extra peak allocation is enough to starve mbedtls. Each pool
    // adapter handles its own field extraction from the resulting
    // JsonDocument, so we cannot attach a generic field filter here
    // without reshaping the MiningPoolInterface contract.
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, *http->getStreamPtr());
    if (err) {
      Serial.printf("Mining pool bad JSON: %s\r\n", err.c_str());
      continue;
    }

    PoolStats stats = poolInterface->parseResponse(doc);
    hashrate = stats.hashrate;
    dailyEarnings = stats.dailyEarnings ? *stats.dailyEarnings : 0;

    if (workQueue != nullptr && (ScreenHandler::getCurrentScreen() ==
                                     SCREEN_MINING_POOL_STATS_HASHRATE ||
                                 ScreenHandler::getCurrentScreen() ==
                                     SCREEN_MINING_POOL_STATS_EARNINGS)) {
      WorkItem priceUpdate = {TASK_MINING_POOL_STATS_UPDATE, 0};
      xQueueSend(workQueue, &priceUpdate, portMAX_DELAY);
    }
  }
}

void MiningPoolStatsFetch::downloadLogoTask() {
  std::string poolName =
      preferences.getString("miningPoolName", DEFAULT_MINING_POOL_NAME).c_str();
  auto *poolInterface = getPool();
  if (!poolInterface)
    return;

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
  xTaskCreate(downloadLogoTaskWrapper, "logoDownload", (6 * 1024), NULL,
              tskIDLE_PRIORITY, NULL);

  xTaskCreate(taskWrapper, "miningPoolStatsFetch", (6 * 1024), NULL,
              tskIDLE_PRIORITY, &taskHandle);

  setMiningPoolTaskHandleForIsr(taskHandle);
}
