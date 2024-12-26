#include "price_notify.hpp"

const char *wsOwnServerPrice = "wss://ws.btclock.dev/ws?assets=bitcoin";
const char *wsOwnServerV2 = "wss://ws-staging.btclock.dev/api/v2/ws";

const char *wsServerPrice = "wss://ws.coincap.io/prices?assets=bitcoin";

// WebsocketsClient client;
esp_websocket_client_handle_t clientPrice = NULL;
esp_websocket_client_config_t config;
uint currentPrice = 90000;
unsigned long int lastPriceUpdate;
bool priceNotifyInit = false;
std::map<char, std::uint64_t> currencyMap;
std::map<char, unsigned long int> lastUpdateMap;
WebSocketsClient priceNotifyWs;

void setupPriceNotify()
{
  if (preferences.getBool("ownDataSource", DEFAULT_OWN_DATA_SOURCE))
  {
    config = {.uri = wsOwnServerPrice,
              .user_agent = USER_AGENT};
  }
  else
  {
    config = {.uri = wsServerPrice,
              .user_agent = USER_AGENT};
    config.cert_pem = isrg_root_x1cert;

    config.task_stack = (6*1024);
  }

  clientPrice = esp_websocket_client_init(&config);
  esp_websocket_register_events(clientPrice, WEBSOCKET_EVENT_ANY,
                                onWebsocketPriceEvent, clientPrice);
  esp_websocket_client_start(clientPrice);

  // priceNotifyWs.beginSSL("ws.coincap.io", 443, "/prices?assets=bitcoin");
  // priceNotifyWs.onEvent(onWebsocketPriceEvent);
  // priceNotifyWs.setReconnectInterval(5000);
  // priceNotifyWs.enableHeartbeat(15000, 3000, 2);
}


// void onWebsocketPriceEvent(WStype_t type, uint8_t * payload, size_t length) {
//     switch(type) {
// 		case WStype_DISCONNECTED:
// 			Serial.printf("[WSc] Disconnected!\n");
// 			break;
// 		case WStype_CONNECTED:
//         {
// 			Serial.printf("[WSc] Connected to url: %s\n", payload);


// 			break;
//         }
// 		case WStype_TEXT:
//         String message = String((char*)payload);
//         onWebsocketPriceMessage(message);
// 		break;
// 		case WStype_BIN:
// 			break;
// 		case WStype_ERROR:			
// 		case WStype_FRAGMENT_TEXT_START:
// 		case WStype_FRAGMENT_BIN_START:
// 		case WStype_FRAGMENT:
//         case WStype_PING:
//         case WStype_PONG:
// 		case WStype_FRAGMENT_FIN:
// 			break;
// 	}
// }

void onWebsocketPriceEvent(void *handler_args, esp_event_base_t base,
                           int32_t event_id, void *event_data)
{
  esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;

  switch (event_id)
  {
  case WEBSOCKET_EVENT_CONNECTED:
    Serial.println("Connected to " + String(config.uri) + " WebSocket");
    priceNotifyInit = true;

    break;
  case WEBSOCKET_EVENT_DATA:
    onWebsocketPriceMessage(data);
    if (preferences.getBool("ownDataSource", DEFAULT_OWN_DATA_SOURCE))
    {
      onWebsocketBlockMessage(data);
    }
    break;
  case WEBSOCKET_EVENT_ERROR:
    Serial.println(F("Price WS Connnection error"));
    break;
  case WEBSOCKET_EVENT_DISCONNECTED:
    Serial.println(F("Price WS Connnection Closed"));
    break;
  }
}

void onWebsocketPriceMessage(esp_websocket_event_data_t *event_data)
{
  JsonDocument doc;

  deserializeJson(doc, (char *)event_data->data_ptr);

  if (doc.containsKey("bitcoin"))
  {
    if (currentPrice != doc["bitcoin"].as<long>())
    {
      processNewPrice(doc["bitcoin"].as<long>(), CURRENCY_USD);
    }
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
    //   const unsigned long oldPrice = currentPrice;
    currencyMap[currency] = newPrice;
    if (currency == CURRENCY_USD && ( lastUpdateMap[currency] == 0 ||
        (currentTime - lastUpdateMap[currency]) > 120))
    {
      preferences.putUInt("lastPrice", currentPrice);
    }
    lastUpdateMap[currency] = currentTime;
    // if (abs((int)(oldPrice-currentPrice)) > round(0.0015*oldPrice)) {
    if (workQueue != nullptr && (ScreenHandler::getCurrentScreen() == SCREEN_BTC_TICKER ||
        ScreenHandler::getCurrentScreen() == SCREEN_SATS_PER_CURRENCY ||
        ScreenHandler::getCurrentScreen() == SCREEN_MARKET_CAP))
    {
      WorkItem priceUpdate = {TASK_PRICE_UPDATE, currency};
      xQueueSend(workQueue, &priceUpdate, portMAX_DELAY);
    }
    //}
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
  if (clientPrice == NULL)
    return false;
  return esp_websocket_client_is_connected(clientPrice);
}

bool getPriceNotifyInit()
{
  return priceNotifyInit;
}

void stopPriceNotify()
{
  if (clientPrice == NULL)
    return;
  esp_websocket_client_close(clientPrice, pdMS_TO_TICKS(5000));
  esp_websocket_client_stop(clientPrice);
  esp_websocket_client_destroy(clientPrice);

  clientPrice = NULL;
}

void restartPriceNotify()
{
  stopPriceNotify();
  if (clientPrice == NULL)
  {
    setupPriceNotify();
    return;
  }
  // esp_websocket_client_close(clientPrice, pdMS_TO_TICKS(5000));
  // esp_websocket_client_stop(clientPrice);
  // esp_websocket_client_start(clientPrice);
}