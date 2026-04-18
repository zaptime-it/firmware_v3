#include "price_notify.hpp"

#include <mutex>

#include "price_policy.hpp"

const char *wsServerPrice = "wss://ws.kraken.com/v2";

WebSocketsClient webSocket;
uint currentPrice = 90000;
unsigned long int lastPriceUpdate;
bool priceNotifyInit = false;
std::map<char, std::uint64_t> currencyMap;
std::map<char, unsigned long int> lastUpdateMap;
// currencyMap and lastUpdateMap are written from the Kraken WS callback
// (priceNotify task) and read from the webserver/SSE/display tasks.
// std::map is not thread-safe and concurrent read/write during tree
// rebalancing can crash. Protect both maps with one mutex.
static std::mutex priceMapMutex;
TaskHandle_t priceNotifyTaskHandle;

void onWebsocketPriceEvent(WStype_t type, uint8_t * payload, size_t length);

void setupPriceNotify()
{
  webSocket.beginSSL("ws.kraken.com", 443, "/v2");
  webSocket.onEvent([](WStype_t type, uint8_t * payload, size_t length) {
    onWebsocketPriceEvent(type, payload, length);
  });
  webSocket.setReconnectInterval(5000);
  webSocket.enableHeartbeat(15000, 3000, 2);

  setupPriceNotifyTask();
}

void onWebsocketPriceEvent(WStype_t type, uint8_t * payload, size_t length) {
    switch(type) {
        case WStype_DISCONNECTED:
            priceNotifyInit = false;
            Serial.println(F("Price WS Connection Closed"));
            break;
        case WStype_CONNECTED:
        {
            priceNotifyInit = true;
            Serial.println("Connected to " + String(wsServerPrice));

            // Subscribe to BTC/<currency> for every currency the user has
            // enabled, not just USD. Kraken accepts an array of symbols in
            // a single subscribe frame; responses carry their own "symbol"
            // field which we use below to dispatch into the per-currency
            // bucket.
            JsonDocument doc;
            doc["method"] = "subscribe";
            JsonObject params = doc["params"].to<JsonObject>();
            params["channel"] = "ticker";
            JsonArray symbolArr = params["symbol"].to<JsonArray>();
            std::string actCurrencies = preferences
                .getString("actCurrencies", DEFAULT_ACTIVE_CURRENCIES)
                .c_str();
            auto codes = price_policy::parseCurrencyCsv(actCurrencies);
            if (codes.empty()) {
                // Never send an empty symbol list — Kraken would reject it
                // and leave us with no feed. USD is the universal fallback.
                codes.push_back("USD");
            }
            for (const auto &code : codes) {
                symbolArr.add(std::string("BTC/") + code);
            }
            webSocket.sendTXT(doc.as<String>().c_str());
            break;
        }
        case WStype_TEXT:
        {
            if (payload == nullptr || length == 0) {
                break;
            }
            JsonDocument doc;
            DeserializationError err = deserializeJson(doc, (char *)payload, length);
            if (err) {
                Serial.printf("Price WS bad JSON: %s\r\n", err.c_str());
                break;
            }

            JsonArray dataArr = doc["data"].as<JsonArray>();
            if (dataArr.isNull()) {
                break;
            }
            for (JsonObject tick : dataArr) {
                if (!tick["last"].is<float>()) continue;
                float price = tick["last"].as<float>();
                uint roundedPrice = round(price);
                // Kraken v2 ticker responses carry the pair in "symbol"
                // as "BTC/<code>"; strip the prefix and map to the char
                // constants the rest of the firmware keys on.
                std::string sym = tick["symbol"].as<std::string>();
                char currency = CURRENCY_USD;
                if (sym.size() >= 7 && sym.compare(0, 4, "BTC/") == 0) {
                    currency = getCurrencyChar(sym.substr(4));
                }
                processNewPrice(roundedPrice, currency);
            }
            break;
        }
        case WStype_BIN:
            break;
        case WStype_ERROR:            
        case WStype_FRAGMENT_TEXT_START:
        case WStype_FRAGMENT_BIN_START:
        case WStype_FRAGMENT:
        case WStype_PING:
        case WStype_PONG:
        case WStype_FRAGMENT_FIN:
            break;
    }
}

void processNewPrice(uint newPrice, char currency)
{
  uint minSecPriceUpd = preferences.getUInt(
      "minSecPriceUpd", DEFAULT_SECONDS_BETWEEN_PRICE_UPDATE);
  uint currentTime = esp_timer_get_time() / 1000000;

  bool wroteToPreferences = false;
  bool shouldQueueWork = false;
  {
    std::lock_guard<std::mutex> lock(priceMapMutex);
    auto it = lastUpdateMap.find(currency);
    if (it != lastUpdateMap.end() && (currentTime - it->second) <= minSecPriceUpd) {
      return;
    }

    currencyMap[currency] = newPrice;

    // Store price in preferences if enough time has passed
    if (it == lastUpdateMap.end() || it->second == 0 ||
        (currentTime - it->second) > 120)
    {
      wroteToPreferences = true;
    }

    lastUpdateMap[currency] = currentTime;
    shouldQueueWork = true;
  }

  if (wroteToPreferences)
  {
    String prefKey = String("lastPrice_") + getCurrencyCode(currency).c_str();
    preferences.putUInt(prefKey.c_str(), newPrice);
  }

  if (shouldQueueWork && workQueue != nullptr &&
      (ScreenHandler::getCurrentScreen() == SCREEN_BTC_TICKER ||
       ScreenHandler::getCurrentScreen() == SCREEN_SATS_PER_CURRENCY ||
       ScreenHandler::getCurrentScreen() == SCREEN_MARKET_CAP))
  {
    WorkItem priceUpdate = {TASK_PRICE_UPDATE, currency};
    xQueueSend(workQueue, &priceUpdate, portMAX_DELAY);
  }
}

void loadStoredPrices()
{
  std::vector<std::string> currencies = getAvailableCurrencies();

  std::lock_guard<std::mutex> lock(priceMapMutex);
  for (const std::string &currency : currencies) {
    String prefKey = String("lastPrice_") + currency.c_str();
    uint storedPrice = preferences.getUInt(prefKey.c_str(), 0);

    if (storedPrice > 0) {
      currencyMap[getCurrencyChar(currency)] = storedPrice;
      // Initialize lastUpdateMap to 0 so next update will store immediately
      lastUpdateMap[getCurrencyChar(currency)] = 0;
    }
  }
}

uint getLastPriceUpdate(char currency)
{
  std::lock_guard<std::mutex> lock(priceMapMutex);
  auto it = lastUpdateMap.find(currency);
  return (it == lastUpdateMap.end()) ? 0 : it->second;
}

uint getPrice(char currency)
{
  std::lock_guard<std::mutex> lock(priceMapMutex);
  auto it = currencyMap.find(currency);
  return (it == currencyMap.end()) ? 0 : static_cast<uint>(it->second);
}

void setPrice(uint newPrice, char currency)
{
  std::lock_guard<std::mutex> lock(priceMapMutex);
  currencyMap[currency] = newPrice;
}

bool isPriceNotifyConnected()
{
  return webSocket.isConnected();
}

bool getPriceNotifyInit()
{
  return priceNotifyInit;
}

void stopPriceNotify()
{
  webSocket.disconnect();
  if (priceNotifyTaskHandle != NULL) {
    vTaskDelete(priceNotifyTaskHandle);
    priceNotifyTaskHandle = NULL;
  }
}

void restartPriceNotify()
{
  stopPriceNotify();
  setupPriceNotify();
}

void taskPriceNotify(void *pvParameters)
{
  for (;;)
  {
    webSocket.loop();
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void setupPriceNotifyTask()
{
  xTaskCreate(taskPriceNotify, "priceNotify", (6 * 1024), NULL, tskIDLE_PRIORITY,
              &priceNotifyTaskHandle);
}

unsigned long PriceNotifyService::lastUpdateSeconds() const {
    return static_cast<unsigned long>(getLastPriceUpdate(CURRENCY_USD));
}

unsigned long PriceNotifyService::staleAfterSeconds() const {
    // Give the publisher 5 missed update windows before restarting. Matches
    // the prior bespoke "5 missed price updates" rule in monitorDataConnections.
    return static_cast<unsigned long>(
        preferences.getUInt("minSecPriceUpd", DEFAULT_SECONDS_BETWEEN_PRICE_UPDATE)) * 5UL;
}