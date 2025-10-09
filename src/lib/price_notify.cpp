#include "price_notify.hpp"

const char *wsServerPrice = "wss://ws.kraken.com/v2";

WebSocketsClient webSocket;
uint currentPrice = 90000;
unsigned long int lastPriceUpdate;
bool priceNotifyInit = false;
std::map<char, std::uint64_t> currencyMap;
std::map<char, unsigned long int> lastUpdateMap;
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
            Serial.println(F("Price WS Connection Closed"));
            break;
        case WStype_CONNECTED:
        {
            Serial.println("Connected to " + String(wsServerPrice));

            JsonDocument doc;
            doc["method"] = "subscribe";
            JsonObject params = doc["params"].to<JsonObject>();
            params["channel"] = "ticker";
            params["symbol"][0] = "BTC/USD";
            
            webSocket.sendTXT(doc.as<String>().c_str());
            break;
        }
        case WStype_TEXT:
        {
            JsonDocument doc;
            deserializeJson(doc, (char *)payload);

            if (doc["data"][0].is<JsonObject>())
            {
                float price = doc["data"][0]["last"].as<float>();
                uint roundedPrice = round(price);
                if (currentPrice != roundedPrice)
                {
                    processNewPrice(roundedPrice, CURRENCY_USD);
                }
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

  if (lastUpdateMap.find(currency) == lastUpdateMap.end() ||
      (currentTime - lastUpdateMap[currency]) > minSecPriceUpd)
  {
    currencyMap[currency] = newPrice;
    
    // Store price in preferences if enough time has passed
    if (lastUpdateMap[currency] == 0 || (currentTime - lastUpdateMap[currency]) > 120)
    {
      String prefKey = String("lastPrice_") + getCurrencyCode(currency).c_str();
      preferences.putUInt(prefKey.c_str(), newPrice);
    }
    
    lastUpdateMap[currency] = currentTime;

    if (workQueue != nullptr && (ScreenHandler::getCurrentScreen() == SCREEN_BTC_TICKER ||
        ScreenHandler::getCurrentScreen() == SCREEN_SATS_PER_CURRENCY ||
        ScreenHandler::getCurrentScreen() == SCREEN_MARKET_CAP))
    {
      WorkItem priceUpdate = {TASK_PRICE_UPDATE, currency};
      xQueueSend(workQueue, &priceUpdate, portMAX_DELAY);
    }
  }
}

void loadStoredPrices()
{
  // Load prices for all supported currencies
  std::vector<std::string> currencies = getAvailableCurrencies();
  
  for (const std::string &currency : currencies) {
    // Get first character as the currency identifier
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
  if (lastUpdateMap.find(currency) == lastUpdateMap.end())
  {
    return 0;
  }

  return lastUpdateMap[currency];
}

uint getPrice(char currency)
{
  if (currencyMap.find(currency) == currencyMap.end())
  {
    return 0;
  }
  return currencyMap[currency];
}

void setPrice(uint newPrice, char currency)
{
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