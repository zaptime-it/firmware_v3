#include "internal.hpp"

#include "lib/drivers/leds/led_handler.hpp"
#include "lib/system/shared.hpp"

// onApiRestart is referenced from internal.hpp because ota_routes.cpp reuses
// the same restart machinery after a firmware flash completes.
void onApiRestart(AsyncWebServerRequest *request) {
  if (requireHttpAuth(request))
    return;
  // Restart from a dedicated task so the AsyncTCP callback returns and the
  // response can be flushed before esp_restart() tears down wifi.
  request->onDisconnect([]() { scheduleDelayedRestart(500); });

  request->send(HTTP_OK);
  if (events.count())
    events.send("closing");
}

static void onApiActionPause(AsyncWebServerRequest *request) {
  if (requireHttpAuth(request))
    return;
  setTimerActive(false);
  request->send(HTTP_OK);
  notifyEventSourceStatus();
}

static void onApiActionTimerRestart(AsyncWebServerRequest *request) {
  if (requireHttpAuth(request))
    return;
  setTimerActive(true);
  request->send(HTTP_OK);
  notifyEventSourceStatus();
}

static void onApiFullRefresh(AsyncWebServerRequest *request) {
  if (requireHttpAuth(request))
    return;
  EPDManager::getInstance().forceFullRefresh();
  std::array<String, NUM_SCREENS> newEpdContent =
      EPDManager::getInstance().getCurrentContent();
  EPDManager::getInstance().setContent(newEpdContent, true);
  request->send(HTTP_OK);
  notifyEventSourceStatus();
}

static void onApiShowScreen(AsyncWebServerRequest *request) {
  if (requireHttpAuth(request))
    return;
  if (request->hasParam("s")) {
    uint currentScreen = request->getParam("s")->value().toInt();
    ScreenHandler::setCurrentScreen(currentScreen);
  }
  request->send(HTTP_OK);
  notifyEventSourceStatus();
}

static void onApiScreenControl(AsyncWebServerRequest *request) {
  if (requireHttpAuth(request))
    return;
  const String &action = request->url();
  if (action.endsWith("/next"))
    ScreenHandler::nextScreen();
  else if (action.endsWith("/previous"))
    ScreenHandler::previousScreen();
  request->send(HTTP_OK);
  notifyEventSourceStatus();
}

static void onApiShowText(AsyncWebServerRequest *request) {
  if (requireHttpAuth(request))
    return;
  if (request->hasParam("t")) {
    String t = request->getParam("t")->value();
    t.toUpperCase(); // still needed until lowercase glyphs are fixed

    // Clamp to t.length() so we don't read past the String when the caller
    // provided fewer than NUM_SCREENS characters.
    std::array<String, NUM_SCREENS> textEpdContent;
    size_t tLen = t.length();
    if (tLen > NUM_SCREENS)
      tLen = NUM_SCREENS;
    for (size_t i = 0; i < tLen; i++)
      textEpdContent[i] = t[i];

    EPDManager::getInstance().setContent(textEpdContent);
  }
  ScreenHandler::setCurrentScreen(SCREEN_CUSTOM);
  request->send(HTTP_OK);
  notifyEventSourceStatus();
}

static void onApiShowTextAdvanced(AsyncWebServerRequest *request,
                                  JsonVariant &json) {
  if (requireHttpAuth(request))
    return;
  JsonArray screens = json.as<JsonArray>();

  std::array<String, NUM_SCREENS> epdContent;
  // Only take up to NUM_SCREENS entries; the previous code wrote past the
  // array end if the client sent more than NUM_SCREENS screens.
  int i = 0;
  for (JsonVariant s : screens) {
    if (i >= static_cast<int>(NUM_SCREENS))
      break;
    epdContent[i] = s.as<String>();
    i++;
  }

  EPDManager::getInstance().setContent(epdContent);
  ScreenHandler::setCurrentScreen(SCREEN_CUSTOM);
  request->send(HTTP_OK);
  notifyEventSourceStatus();
}

static void onApiShowCurrency(AsyncWebServerRequest *request) {
  if (requireHttpAuth(request))
    return;
  if (!request->hasParam("c")) {
    request->send(HTTP_NOT_FOUND);
    return;
  }

  std::string currency = request->getParam("c")->value().c_str();
  if (!isActiveCurrency(currency)) {
    request->send(HTTP_NOT_FOUND);
    return;
  }

  char curChar = getCurrencyChar(currency);
  ScreenHandler::setCurrentCurrency(curChar);
  ScreenHandler::setCurrentScreen(ScreenHandler::getCurrentScreen());

  request->send(HTTP_OK);
  notifyEventSourceStatus();
}

static void onApiIdentify(AsyncWebServerRequest *request) {
  if (requireHttpAuth(request))
    return;
  getLedHandler().queueEffect(LED_FLASH_IDENTIFY);
  request->send(HTTP_OK);
}

static void onApiSetWifiTxPower(AsyncWebServerRequest *request) {
  if (requireHttpAuth(request))
    return;
  if (request->hasParam("txPower")) {
    int txPower = request->getParam("txPower")->value().toInt();
    if (static_cast<int>(wifi_power_t::WIFI_POWER_MINUS_1dBm) <= txPower &&
        txPower <= static_cast<int>(wifi_power_t::WIFI_POWER_19_5dBm)) {
      if (WiFi.setTxPower(static_cast<wifi_power_t>(txPower))) {
        preferences.putInt("txPower", txPower);
        request->send(HTTP_OK, "application/json", "{\"setTxPower\": \"ok\"}");
        return;
      }
    }
  }
  request->send(HTTP_BAD_REQUEST);
}

static void onApiStopDataSources(AsyncWebServerRequest *request) {
  if (requireHttpAuth(request))
    return;
  AsyncResponseStream *response = request->beginResponseStream(JSON_CONTENT);

  stopPriceNotify();
  BlockNotify::getInstance().stop();

  request->send(response);
  notifyEventSourceStatus();
}

static void onApiRestartDataSources(AsyncWebServerRequest *request) {
  if (requireHttpAuth(request))
    return;
  AsyncResponseStream *response = request->beginResponseStream(JSON_CONTENT);

  if (getDataSource() == BTCLOCK_SOURCE || getDataSource() == CUSTOM_SOURCE) {
    V2Notify::restartV2Notify();
  } else if (getDataSource() == THIRD_PARTY_SOURCE) {
    BlockNotify::getInstance().restart();
    restartPriceNotify();
  }

  request->send(response);
  notifyEventSourceStatus();
}

void registerActionRoutes() {
  // State-changing endpoints live behind POST so they stop showing up in
  // browser history, bookmark warmers, and <link rel="prefetch"> runs. The
  // old GET routes are intentionally not registered; the WebUI is being
  // rebuilt against 3.4.0.
  server.on("/api/wifi_set_tx_power", AsyncWebRequestMethod::HTTP_POST, onApiSetWifiTxPower);
  server.on("/api/full_refresh", AsyncWebRequestMethod::HTTP_POST, onApiFullRefresh);
  server.on("/api/stop_datasources", AsyncWebRequestMethod::HTTP_POST, onApiStopDataSources);
  server.on("/api/restart_datasources", AsyncWebRequestMethod::HTTP_POST, onApiRestartDataSources);
  server.on("/api/action/pause", AsyncWebRequestMethod::HTTP_POST, onApiActionPause);
  server.on("/api/action/timer_restart", AsyncWebRequestMethod::HTTP_POST, onApiActionTimerRestart);
  server.on("/api/show/screen", AsyncWebRequestMethod::HTTP_POST, onApiShowScreen);
  server.on("/api/show/currency", AsyncWebRequestMethod::HTTP_POST, onApiShowCurrency);
  server.on("/api/show/text", AsyncWebRequestMethod::HTTP_POST, onApiShowText);
  server.on("/api/screen/next", AsyncWebRequestMethod::HTTP_POST, onApiScreenControl);
  server.on("/api/screen/previous", AsyncWebRequestMethod::HTTP_POST, onApiScreenControl);
  server.on("/api/identify", AsyncWebRequestMethod::HTTP_POST, onApiIdentify);
  server.on("/api/restart", AsyncWebRequestMethod::HTTP_POST, onApiRestart);

  AsyncCallbackJsonWebHandler *handler = new AsyncCallbackJsonWebHandler(
      "/api/show/custom", onApiShowTextAdvanced);
  handler->setMethod(AsyncWebRequestMethod::HTTP_POST);
  server.addHandler(handler);

  server.addRewrite(new OneParamRewrite("/api/show/currency/{c}",
                                        "/api/show/currency?c={c}"));
  server.addRewrite(
      new OneParamRewrite("/api/show/screen/{s}", "/api/show/screen?s={s}"));
  server.addRewrite(
      new OneParamRewrite("/api/show/text/{text}", "/api/show/text?t={text}"));
  // Placeholder name in the rewrite target must match the pattern token.
  // Previously the template used {text} so the rewrite produced a literal
  // "?t={text}" URL instead of substituting the captured number.
  server.addRewrite(new OneParamRewrite("/api/show/number/{number}",
                                        "/api/show/text?t={number}"));
}
