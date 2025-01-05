#include "webserver.hpp"
#include "lib/led_handler.hpp"
#include "lib/shared.hpp"

static const char* JSON_CONTENT = "application/json";

static const char *const PROGMEM strSettings[] = {
    "hostnamePrefix", "mempoolInstance", "nostrPubKey", "nostrRelay", "bitaxeHostname", "miningPoolName", "miningPoolUser", "nostrZapPubkey", "httpAuthUser", "httpAuthPass", "gitReleaseUrl", "poolLogosUrl", "ceEndpoint", "fontName"};

static const char *const PROGMEM uintSettings[] = {"minSecPriceUpd", "fullRefreshMin", "ledBrightness", "flMaxBrightness", "flEffectDelay", "luxLightToggle", "wpTimeout"};

static const char *const PROGMEM boolSettings[] = {"ledTestOnPower", "ledFlashOnUpd",
                                                   "mdnsEnabled", "otaEnabled", "stealFocus",
                                                   "mcapBigChar", "useSatsSymbol", "useBlkCountdown",
                                                   "suffixPrice", "disableLeds", 
                                                   "mowMode", "suffixShareDot", "flOffWhenDark",
                                                   "flAlwaysOn", "flDisable", "flFlashOnUpd",
                                                   "mempoolSecure", "bitaxeEnabled",
                                                   "miningPoolStats", "verticalDesc",
                                                   "nostrZapNotify", "httpAuthEnabled",
                                                   "enableDebugLog", "ceDisableSSL", "dndEnabled", "dndTimeBasedEnabled"};

AsyncWebServer server(80);
AsyncEventSource events("/events");
TaskHandle_t eventSourceTaskHandle;

#define HTTP_OK 200
#define HTTP_BAD_REQUEST 400

void setupWebserver()
{
  events.onConnect([](AsyncEventSourceClient *client)
                   { client->send("welcome", NULL, millis(), 1000); });
  server.addHandler(&events);

  AsyncStaticWebHandler &staticHandler = server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

  server.rewrite("/convert", "/");
  server.rewrite("/api", "/");

  if (preferences.getBool("httpAuthEnabled", DEFAULT_HTTP_AUTH_ENABLED))
  {
    staticHandler.setAuthentication(
        preferences.getString("httpAuthUser", DEFAULT_HTTP_AUTH_USERNAME),
        preferences.getString("httpAuthPass", DEFAULT_HTTP_AUTH_PASSWORD));
  }
  //  server.on("/", HTTP_GET, onIndex);
  server.on("/api/status", HTTP_GET, onApiStatus);
  server.on("/api/system_status", HTTP_GET, onApiSystemStatus);
  server.on("/api/wifi_set_tx_power", HTTP_GET, onApiSetWifiTxPower);

  server.on("/api/full_refresh", HTTP_GET, onApiFullRefresh);

  server.on("/api/stop_datasources", HTTP_GET, onApiStopDataSources);
  server.on("/api/restart_datasources", HTTP_GET, onApiRestartDataSources);

  server.on("/api/action/pause", HTTP_GET, onApiActionPause);
  server.on("/api/action/timer_restart", HTTP_GET, onApiActionTimerRestart);

  server.on("/api/settings", HTTP_GET, onApiSettingsGet);

  server.on("/api/show/screen", HTTP_GET, onApiShowScreen);
  server.on("/api/show/currency", HTTP_GET, onApiShowCurrency);

  server.on("/api/show/text", HTTP_GET, onApiShowText);

  server.on("/api/screen/next", HTTP_GET, onApiScreenControl);
  server.on("/api/screen/previous", HTTP_GET, onApiScreenControl);

  AsyncCallbackJsonWebHandler *settingsPatchHandler =
      new AsyncCallbackJsonWebHandler("/api/json/settings", onApiSettingsPatch);
  server.addHandler(settingsPatchHandler);

  AsyncCallbackJsonWebHandler *handler = new AsyncCallbackJsonWebHandler(
      "/api/show/custom", onApiShowTextAdvanced);
  server.addHandler(handler);

  AsyncCallbackJsonWebHandler *lightsJsonHandler =
      new AsyncCallbackJsonWebHandler("/api/lights/set", onApiLightsSetJson);
  server.addHandler(lightsJsonHandler);

  server.on("/api/lights/off", HTTP_GET, onApiLightsOff);
  server.on("/api/lights/color", HTTP_GET, onApiLightsSetColor);
  server.on("/api/lights", HTTP_GET, onApiLightsStatus);
  server.on("/api/identify", HTTP_GET, onApiIdentify);

#ifdef HAS_FRONTLIGHT
  server.on("/api/frontlight/on", HTTP_GET, onApiFrontlightOn);
  server.on("/api/frontlight/flash", HTTP_GET, onApiFrontlightFlash);
  server.on("/api/frontlight/status", HTTP_GET, onApiFrontlightStatus);

  server.on("/api/frontlight/brightness", HTTP_GET, onApiFrontlightSetBrightness);
  server.on("/api/frontlight/off", HTTP_GET, onApiFrontlightOff);

  server.addRewrite(
      new OneParamRewrite("/api/frontlight/brightness/{b}", "/api/frontlight/brightness?b={b}"));
#endif

  // server.on("^\\/api\\/lights\\/([A-Fa-f0-9]{6}|[A-Fa-f0-9]{3})$", HTTP_GET,
  // onApiLightsSetColor);

  if (preferences.getBool("otaEnabled", DEFAULT_OTA_ENABLED))
  {
    server.on("/upload/firmware", HTTP_POST, onFirmwareUpdate, asyncFirmwareUpdateHandler);
    server.on("/upload/webui", HTTP_POST, onFirmwareUpdate, asyncWebuiUpdateHandler);
    server.on("/api/firmware/auto_update", HTTP_GET, onAutoUpdateFirmware);
  }

  server.on("/api/restart", HTTP_GET, onApiRestart);
  server.addRewrite(
      new OneParamRewrite("/api/show/currency/{c}", "/api/show/currency?c={c}"));
  server.addRewrite(new OneParamRewrite("/api/lights/color/{color}",
                                        "/api/lights/color?c={color}"));
  server.addRewrite(
      new OneParamRewrite("/api/show/screen/{s}", "/api/show/screen?s={s}"));
  server.addRewrite(
      new OneParamRewrite("/api/show/text/{text}", "/api/show/text?t={text}"));
  server.addRewrite(new OneParamRewrite("/api/show/number/{number}",
                                        "/api/show/text?t={text}"));

  server.on("/api/dnd/status", HTTP_GET, onApiDNDStatus);
  server.on("/api/dnd/enable", HTTP_POST, onApiDNDEnable);
  server.on("/api/dnd/disable", HTTP_POST, onApiDNDDisable);

  server.onNotFound(onNotFound);

  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods",
                                       "GET, PATCH, POST, OPTIONS");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "*");

  server.begin();

  if (preferences.getBool("mdnsEnabled", DEFAULT_MDNS_ENABLED))
  {
    if (!MDNS.begin(getMyHostname()))
    {
      Serial.println(F("Error setting up MDNS responder!"));
      while (1)
      {
        delay(1000);
      }
    }
    MDNS.addService("http", "tcp", 80);
    MDNS.addServiceTxt("http", "tcp", "model", "BTClock");
    MDNS.addServiceTxt("http", "tcp", "version", "3.0");
    MDNS.addServiceTxt("http", "tcp", "rev", GIT_REV);
    MDNS.addServiceTxt("http", "tcp", "hw_rev", getHwRev());
  }

  xTaskCreate(eventSourceTask, "eventSourceTask", 4096, NULL, tskIDLE_PRIORITY,
              &eventSourceTaskHandle);
}

void stopWebServer() { server.end(); }

void onFirmwareUpdate(AsyncWebServerRequest *request)
{
  bool shouldReboot = !Update.hasError();
  AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", shouldReboot ? "OK" : "FAIL");
  response->addHeader("Connection", "close");
  request->send(response);
}

void onAutoUpdateFirmware(AsyncWebServerRequest *request)
{
  UpdateMessage msg = {UPDATE_ALL};
  if (xQueueSend(otaQueue, &msg, 0) == pdTRUE)
  {
    request->send(200, "application/json", "{\"msg\":\"Firmware update triggered\"}");
  }
  else
  {
    request->send(503,"application/json", "{\"msg\":\"Update already in progress\"}"); 
  }
}

void asyncWebuiUpdateHandler(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
  asyncFileUpdateHandler(request, filename, index, data, len, final, U_SPIFFS);
}

void asyncFileUpdateHandler(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final, int command)
{
  if (!index)
  {
    Serial.printf("Update Start: %s\n", filename.c_str());

    if (command == U_FLASH)
    {
      // Update.runAsync(true);
      if (!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000), command)
      {
        Update.printError(Serial);
        return;
      }
    }
    else if (command == U_SPIFFS)
    {
      size_t fsSize = UPDATE_SIZE_UNKNOWN; // or specify the size of your filesystem partition
      if (!Update.begin(fsSize, U_SPIFFS)) // or U_FS for LittleFS
      {
        Update.printError(Serial);
        return;
      }
    }
  }
  if (!Update.hasError())
  {
    if (Update.write(data, len) != len)
    {
      Update.printError(Serial);
    }
  }
  if (final)
  {
    if (Update.end(true))
    {
      Serial.printf("Update Success: %uB\n", index + len);
      onApiRestart(request);
    }
    else
    {
      Update.printError(Serial);
    }
  }
}

void asyncFirmwareUpdateHandler(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
  asyncFileUpdateHandler(request, filename, index, data, len, final, U_FLASH);
}

JsonDocument getStatusObject()
{
  auto& ledHandler = getLedHandler();
  JsonDocument root;

  root["currentScreen"] = ScreenHandler::getCurrentScreen();
  root["numScreens"] = NUM_SCREENS;
  root["timerRunning"] = isTimerActive();
  root["isOTAUpdating"] = getIsOTAUpdating();
  root["espUptime"] = esp_timer_get_time() / 1000000;
  root["espFreeHeap"] = ESP.getFreeHeap();
  root["espHeapSize"] = ESP.getHeapSize();

  JsonObject conStatus = root["connectionStatus"].to<JsonObject>();

  conStatus["price"] = isPriceNotifyConnected();
  conStatus["blocks"] = isBlockNotifyConnected();
  conStatus["V2"] = V2Notify::isV2NotifyConnected();
  conStatus["nostr"] = nostrConnected();

  root["rssi"] = WiFi.RSSI();
  root["currency"] = getCurrencyCode(ScreenHandler::getCurrentCurrency());

#ifdef HAS_FRONTLIGHT
  std::vector<uint16_t> statuses = ledHandler.frontlightGetStatus();
  uint16_t arr[NUM_SCREENS];
  std::copy(statuses.begin(), statuses.end(), arr);

  JsonArray data = root["flStatus"].to<JsonArray>();
  copyArray(arr, data);

  if (hasLightLevel())
  {
    root["lightLevel"] = getLightLevel();
  }
#endif

  // Add DND status
  root["dnd"]["enabled"] = ledHandler.isDNDEnabled();
  root["dnd"]["timeBasedEnabled"] = ledHandler.isDNDTimeBasedEnabled();
  root["dnd"]["startTime"] = String(ledHandler.getDNDStartHour()) + ":" + 
                           (ledHandler.getDNDStartMinute() < 10 ? "0" : "") + String(ledHandler.getDNDStartMinute());
  root["dnd"]["endTime"] = String(ledHandler.getDNDEndHour()) + ":" + 
                         (ledHandler.getDNDEndMinute() < 10 ? "0" : "") + String(ledHandler.getDNDEndMinute());
  root["dnd"]["active"] = ledHandler.isDNDActive();
  
  return root;
}

JsonDocument getLedStatusObject()
{
  auto& ledHandler = getLedHandler();
  auto& pixels = ledHandler.getPixels();
  
  JsonDocument root;
  JsonArray colors = root["data"].to<JsonArray>();

  for (uint i = 0; i < pixels.numPixels(); i++)
  {
    uint32_t pixColor = pixels.getPixelColor(pixels.numPixels() - i - 1);
    uint red = (pixColor >> 16) & 0xFF;
    uint green = (pixColor >> 8) & 0xFF;
    uint blue = pixColor & 0xFF;
    char hexColor[8];
    snprintf(hexColor, sizeof(hexColor), "#%02X%02X%02X", red, green, blue);
    colors.add(hexColor);
  }

  return root;
}

void eventSourceUpdate() {
    if (!events.count()) return;
    
    JsonDocument doc = getStatusObject();
    doc["leds"] = getLedStatusObject()["data"];

    // Get current EPD content directly as array
    std::array<String, NUM_SCREENS> epdContent = getCurrentEpdContent();
    
    // Add EPD content arrays
    JsonArray data = doc["data"].to<JsonArray>();
    
    // Copy array elements directly
    for(const auto& content : epdContent) {
      data.add(content);
    }

    String buffer;
    serializeJson(doc, buffer);
    events.send(buffer.c_str(), "status");
}

/**
 * @Api
 * @Path("/api/status")
 */
void onApiStatus(AsyncWebServerRequest *request)
{
  AsyncResponseStream *response =
      request->beginResponseStream(JSON_CONTENT);

  JsonDocument root = getStatusObject();
  
  // Get current EPD content directly as array
  std::array<String, NUM_SCREENS> epdContent = getCurrentEpdContent();
  
  // Add EPD content arrays
  JsonArray data = root["data"].to<JsonArray>();
  
  // Copy array elements directly
  for(const auto& content : epdContent) {
    data.add(content);
  }

  root["leds"] = getLedStatusObject()["data"];
  serializeJson(root, *response);

  request->send(response);
}

/**
 * @Api
 * @Path("/api/action/pause")
 */
void onApiActionPause(AsyncWebServerRequest *request)
{
  setTimerActive(false);
  request->send(HTTP_OK);
};

/**
 * @Api
 * @Path("/api/action/timer_restart")
 */
void onApiActionTimerRestart(AsyncWebServerRequest *request)
{
  setTimerActive(true);
  request->send(HTTP_OK);
}

/**
 * @Api
 * @Path("/api/full_refresh")
 */
void onApiFullRefresh(AsyncWebServerRequest *request)
{
  forceFullRefresh();
  std::array<String, NUM_SCREENS> newEpdContent = getCurrentEpdContent();

  setEpdContent(newEpdContent, true);

  request->send(HTTP_OK);
}

/**
 * @Api
 * @Path("/api/show/screen")
 */
void onApiShowScreen(AsyncWebServerRequest *request)
{
  if (request->hasParam("s"))
  {
    const AsyncWebParameter *p = request->getParam("s");
    uint currentScreen = p->value().toInt();
    ScreenHandler::setCurrentScreen(currentScreen);
  }
  request->send(HTTP_OK);
}

/**
 * @Api
 * @Path("/api/screen/next")
 */
void onApiScreenControl(AsyncWebServerRequest *request) {
    const String& action = request->url();
    if (action.endsWith("/next")) {
        ScreenHandler::nextScreen();
    } else if (action.endsWith("/previous")) {
        ScreenHandler::previousScreen();
    }
    request->send(HTTP_OK);
}

void onApiShowText(AsyncWebServerRequest *request)
{
  if (request->hasParam("t"))
  {
    const AsyncWebParameter *p = request->getParam("t");
    String t = p->value();
    t.toUpperCase(); // This is needed as long as lowercase letters are glitchy

    std::array<String, NUM_SCREENS> textEpdContent;
    for (uint i = 0; i < NUM_SCREENS; i++)
    {
      textEpdContent[i] = t[i];
    }

    setEpdContent(textEpdContent);
  }
  ScreenHandler::setCurrentScreen(SCREEN_CUSTOM);
  request->send(HTTP_OK);
}

void onApiShowTextAdvanced(AsyncWebServerRequest *request, JsonVariant &json)
{
  JsonArray screens = json.as<JsonArray>();

  std::array<String, NUM_SCREENS> epdContent;
  int i = 0;
  for (JsonVariant s : screens)
  {
    epdContent[i] = s.as<String>();
    i++;
  }

  setEpdContent(epdContent);

  ScreenHandler::setCurrentScreen(SCREEN_CUSTOM);
  request->send(HTTP_OK);
}

void onApiSettingsPatch(AsyncWebServerRequest *request, JsonVariant &json)
{
  if (
      preferences.getBool("httpAuthEnabled", DEFAULT_HTTP_AUTH_ENABLED) &&
      !request->authenticate(
          preferences.getString("httpAuthUser", DEFAULT_HTTP_AUTH_USERNAME).c_str(),
          preferences.getString("httpAuthPass", DEFAULT_HTTP_AUTH_PASSWORD).c_str()))
  {
    return request->requestAuthentication();
  }

  JsonObject settings = json.as<JsonObject>();

  bool settingsChanged = true;

  if (settings["invertedColor"].is<bool>())
  {
    bool inverted = settings["invertedColor"].as<bool>();
    preferences.putBool("invertedColor", inverted);
    if (inverted) {
      preferences.putUInt("fgColor", GxEPD_WHITE);
      preferences.putUInt("bgColor", GxEPD_BLACK);
      setFgColor(GxEPD_WHITE);
      setBgColor(GxEPD_BLACK);
    } else {
      preferences.putUInt("fgColor", GxEPD_BLACK);
      preferences.putUInt("bgColor", GxEPD_WHITE);
      setFgColor(GxEPD_BLACK);
      setBgColor(GxEPD_WHITE);
    }
    Serial.printf("Setting invertedColor to %d\r\n", inverted);
    settingsChanged = true;
  }

  if (settings["timePerScreen"].is<uint>())
  {
    preferences.putUInt("timerSeconds",
                        settings["timePerScreen"].as<uint>() * 60);
  }

  for (String setting : strSettings)
  {
    if (settings[setting].is<String>())
    {
      preferences.putString(setting.c_str(), settings[setting].as<String>());
      Serial.printf("Setting %s to %s\r\n", setting.c_str(),
                    settings[setting].as<String>().c_str());
    }
  }



  for (String setting : uintSettings)
  {
    if (settings[setting].is<uint>())
    {
      preferences.putUInt(setting.c_str(), settings[setting].as<uint>());
      Serial.printf("Setting %s to %d\r\n", setting.c_str(),
                    settings[setting].as<uint>());
    }
  }

  if (settings["tzOffset"].is<int>())
  {
    int gmtOffset = settings["tzOffset"].as<int>() * 60;
    size_t written = preferences.putInt("gmtOffset", gmtOffset);
    Serial.printf("Setting %s to %d (%d minutes, written %d)\r\n", "gmtOffset",
                  gmtOffset, settings["tzOffset"].as<int>(), written);
  }

  for (String setting : boolSettings)
  {
    if (settings[setting].is<bool>())
    {
      preferences.putBool(setting.c_str(), settings[setting].as<bool>());
      Serial.printf("Setting %s to %d\r\n", setting.c_str(),
                    settings[setting].as<bool>());
    }
  }

  if (settings["screens"].is<JsonArray>())
  {
    for (JsonVariant screen : settings["screens"].as<JsonArray>())
    {
      JsonObject s = screen.as<JsonObject>();
      uint id = s["id"].as<uint>();
      String key = "screen[" + String(id) + "]";
      String prefKey = "screen" + String(id) + "Visible";
      bool visible = s["enabled"].as<bool>();
      preferences.putBool(prefKey.c_str(), visible);
    }
  }

  if (settings["actCurrencies"].is<JsonArray>())
  {
    String actCurrencies;

    for (JsonVariant cur : settings["actCurrencies"].as<JsonArray>())
    {
      if (!actCurrencies.isEmpty())
      {
        actCurrencies += ",";
      }
      actCurrencies += cur.as<String>();
    }

    preferences.putString("actCurrencies", actCurrencies.c_str());
    Serial.printf("Set actCurrencies: %s\n", actCurrencies.c_str());
  }

  if (settings["txPower"].is<int>())
  {
    int txPower = settings["txPower"].as<int>();

    if (txPower == 80)
    {
      preferences.remove("txPower");
      if (WiFi.getTxPower() != 80)
      {
        ESP.restart();
      }
    }
    else if (static_cast<int>(wifi_power_t::WIFI_POWER_MINUS_1dBm) <=
                 txPower &&
             txPower <= static_cast<int>(wifi_power_t::WIFI_POWER_19_5dBm))
    {
      // is valid value

      if (WiFi.setTxPower(static_cast<wifi_power_t>(txPower)))
      {
        Serial.printf("Set WiFi Tx power to: %d\n", txPower);
        preferences.putInt("txPower", txPower);
        settingsChanged = true;
      }
    }
  }

  // Handle data source setting
  if (settings["dataSource"].is<uint8_t>()) {
    uint8_t dataSource = settings["dataSource"].as<uint8_t>();
    if (dataSource <= CUSTOM_SOURCE) { // Validate including custom source
      preferences.putUChar("dataSource", dataSource);
      Serial.printf("Setting dataSource to %d\r\n", dataSource);
      settingsChanged = true;
    }
  }

  // Handle custom endpoint settings
  if (settings["customEndpoint"].is<String>()) {
    preferences.putString("customEndpoint", settings["customEndpoint"].as<String>());
    Serial.printf("Setting customEndpoint to %s\r\n", settings["customEndpoint"].as<String>().c_str());
    settingsChanged = true;
  }

  if (settings["customEndpointDisableSSL"].is<bool>()) {
    preferences.putBool("customEndpointDisableSSL", settings["customEndpointDisableSSL"].as<bool>());
    Serial.printf("Setting customEndpointDisableSSL to %d\r\n", settings["customEndpointDisableSSL"].as<bool>());
    settingsChanged = true;
  }

  // Handle DND settings
  if (settings.containsKey("dnd")) {
    JsonObject dndObj = settings["dnd"];
    auto& ledHandler = getLedHandler();
    
    if (dndObj.containsKey("timeBasedEnabled")) {
      ledHandler.setDNDTimeBasedEnabled(dndObj["timeBasedEnabled"].as<bool>());
    }
    if (dndObj.containsKey("startHour") && dndObj.containsKey("startMinute") &&
        dndObj.containsKey("endHour") && dndObj.containsKey("endMinute")) {
      ledHandler.setDNDTimeRange(
          dndObj["startHour"].as<uint8_t>(),
          dndObj["startMinute"].as<uint8_t>(),
          dndObj["endHour"].as<uint8_t>(),
          dndObj["endMinute"].as<uint8_t>());
    }
  }

  request->send(HTTP_OK);
  if (settingsChanged)
  {
    auto& ledHandler = getLedHandler();
    ledHandler.queueEffect(LED_FLASH_SUCCESS);
  }
}

void onApiRestart(AsyncWebServerRequest *request)
{
  request->onDisconnect([]() {
    delay(500);

    noInterrupts();
    esp_restart();
  });

  request->send(HTTP_OK);

  if (events.count())
    events.send("closing");
}

void onApiIdentify(AsyncWebServerRequest *request)
{
  auto& ledHandler = getLedHandler();
  ledHandler.queueEffect(LED_FLASH_IDENTIFY);

  request->send(HTTP_OK);
}

/**
 * @Api
 * @Method GET
 * @Path("/api/settings")
 */
void onApiSettingsGet(AsyncWebServerRequest *request)
{
  if (
      preferences.getBool("httpAuthEnabled", DEFAULT_HTTP_AUTH_ENABLED) &&
      !request->authenticate(
          preferences.getString("httpAuthUser", DEFAULT_HTTP_AUTH_USERNAME).c_str(),
          preferences.getString("httpAuthPass", DEFAULT_HTTP_AUTH_PASSWORD).c_str()))
  {
    return request->requestAuthentication();
  }

  JsonDocument root;
  root["numScreens"] = NUM_SCREENS;
  root["invertedColor"] = preferences.getBool("invertedColor", getFgColor() == GxEPD_WHITE);
  root["timerSeconds"] = getTimerSeconds();
  root["timerRunning"] = isTimerActive();
  root["minSecPriceUpd"] = preferences.getUInt(
      "minSecPriceUpd", DEFAULT_SECONDS_BETWEEN_PRICE_UPDATE);
  root["fullRefreshMin"] =
      preferences.getUInt("fullRefreshMin", DEFAULT_MINUTES_FULL_REFRESH);
  root["wpTimeout"] = preferences.getUInt("wpTimeout", DEFAULT_WP_TIMEOUT);
  root["tzOffset"] = preferences.getInt("gmtOffset", DEFAULT_TIME_OFFSET_SECONDS) / 60;
  
  // Add data source settings
  root["dataSource"] = preferences.getUChar("dataSource", DEFAULT_DATA_SOURCE);
  
  // Mempool settings (only used for THIRD_PARTY_SOURCE)
  root["mempoolInstance"] = preferences.getString("mempoolInstance", DEFAULT_MEMPOOL_INSTANCE);
  root["mempoolSecure"] = preferences.getBool("mempoolSecure", DEFAULT_MEMPOOL_SECURE);
  
  // Nostr settings (used for NOSTR_SOURCE or when zapNotify is enabled)
  root["nostrPubKey"] = preferences.getString("nostrPubKey", DEFAULT_NOSTR_NPUB);
  root["nostrRelay"] = preferences.getString("nostrRelay", DEFAULT_NOSTR_RELAY);
  root["nostrZapNotify"] = preferences.getBool("nostrZapNotify", DEFAULT_ZAP_NOTIFY_ENABLED);
  root["nostrZapPubkey"] = preferences.getString("nostrZapPubkey", DEFAULT_ZAP_NOTIFY_PUBKEY);
  root["ledFlashOnZap"] = preferences.getBool("ledFlashOnZap", DEFAULT_LED_FLASH_ON_ZAP);
  root["fontName"] = preferences.getString("fontName", DEFAULT_FONT_NAME);
  root["availableFonts"] = FontNames::getAvailableFonts();
  // Custom endpoint settings (only used for CUSTOM_SOURCE)
  root["customEndpoint"] = preferences.getString("customEndpoint", DEFAULT_CUSTOM_ENDPOINT);
  root["customEndpointDisableSSL"] = preferences.getBool("customEndpointDisableSSL", DEFAULT_CUSTOM_ENDPOINT_DISABLE_SSL);

  root["ledTestOnPower"] = preferences.getBool("ledTestOnPower", DEFAULT_LED_TEST_ON_POWER);
  root["ledFlashOnUpd"] = preferences.getBool("ledFlashOnUpd", DEFAULT_LED_FLASH_ON_UPD);
  root["ledBrightness"] = preferences.getUInt("ledBrightness", DEFAULT_LED_BRIGHTNESS);
  root["stealFocus"] = preferences.getBool("stealFocus", DEFAULT_STEAL_FOCUS);
  root["mcapBigChar"] = preferences.getBool("mcapBigChar", DEFAULT_MCAP_BIG_CHAR);
  root["mdnsEnabled"] = preferences.getBool("mdnsEnabled", DEFAULT_MDNS_ENABLED);
  root["otaEnabled"] = preferences.getBool("otaEnabled", DEFAULT_OTA_ENABLED);
  root["useSatsSymbol"] = preferences.getBool("useSatsSymbol", DEFAULT_USE_SATS_SYMBOL);
  root["useBlkCountdown"] = preferences.getBool("useBlkCountdown", DEFAULT_USE_BLOCK_COUNTDOWN);
  root["suffixPrice"] = preferences.getBool("suffixPrice", DEFAULT_SUFFIX_PRICE);
  root["disableLeds"] = preferences.getBool("disableLeds", DEFAULT_DISABLE_LEDS);
  root["mowMode"] = preferences.getBool("mowMode", DEFAULT_MOW_MODE);
  root["verticalDesc"] = preferences.getBool("verticalDesc", DEFAULT_VERTICAL_DESC);

  root["suffixShareDot"] = preferences.getBool("suffixShareDot", DEFAULT_SUFFIX_SHARE_DOT);
  root["enableDebugLog"] = preferences.getBool("enableDebugLog", DEFAULT_ENABLE_DEBUG_LOG);

  root["hostnamePrefix"] = preferences.getString("hostnamePrefix", DEFAULT_HOSTNAME_PREFIX);
  root["hostname"] = getMyHostname();
  root["ip"] = WiFi.localIP();
  root["txPower"] = WiFi.getTxPower();

  root["gitReleaseUrl"] = preferences.getString("gitReleaseUrl", DEFAULT_GIT_RELEASE_URL);

  root["bitaxeEnabled"] = preferences.getBool("bitaxeEnabled", DEFAULT_BITAXE_ENABLED);
  root["bitaxeHostname"] = preferences.getString("bitaxeHostname", DEFAULT_BITAXE_HOSTNAME);

  root["miningPoolStats"] = preferences.getBool("miningPoolStats", DEFAULT_MINING_POOL_STATS_ENABLED);
  root["miningPoolName"] = preferences.getString("miningPoolName", DEFAULT_MINING_POOL_NAME);
  root["miningPoolUser"] = preferences.getString("miningPoolUser", DEFAULT_MINING_POOL_USER);
  root["availablePools"] = PoolFactory::getAvailablePools();
  root["httpAuthEnabled"] = preferences.getBool("httpAuthEnabled", DEFAULT_HTTP_AUTH_ENABLED);
  root["httpAuthUser"] = preferences.getString("httpAuthUser", DEFAULT_HTTP_AUTH_USERNAME);
  root["httpAuthPass"] = preferences.getString("httpAuthPass", DEFAULT_HTTP_AUTH_PASSWORD);
#ifdef HAS_FRONTLIGHT
  root["hasFrontlight"] = true;
  root["flDisable"] = preferences.getBool("flDisable");
  root["flMaxBrightness"] = preferences.getUInt("flMaxBrightness", DEFAULT_FL_MAX_BRIGHTNESS);
  root["flAlwaysOn"] = preferences.getBool("flAlwaysOn", DEFAULT_FL_ALWAYS_ON);
  root["flEffectDelay"] = preferences.getUInt("flEffectDelay", DEFAULT_FL_EFFECT_DELAY);
  root["flFlashOnUpd"] = preferences.getBool("flFlashOnUpd", DEFAULT_FL_FLASH_ON_UPDATE);
  root["flFlashOnZap"] = preferences.getBool("flFlashOnZap", DEFAULT_FL_FLASH_ON_ZAP);

  root["hasLightLevel"] = hasLightLevel();
  root["luxLightToggle"] = preferences.getUInt("luxLightToggle", DEFAULT_LUX_LIGHT_TOGGLE);
  root["flOffWhenDark"] = preferences.getBool("flOffWhenDark", DEFAULT_FL_OFF_WHEN_DARK);

#else
  root["hasFrontlight"] = false;
  root["hasLightLevel"] = false;
#endif

  root["hwRev"] = getHwRev();
  root["fsRev"] = getFsRev();

#ifdef GIT_REV
  root["gitRev"] = String(GIT_REV);
#endif
#ifdef GIT_TAG
  root["gitTag"] = String(GIT_TAG);
#endif

#ifdef LAST_BUILD_TIME
  root["lastBuildTime"] = String(LAST_BUILD_TIME);
#endif
  JsonArray screens = root["screens"].to<JsonArray>();

  root["actCurrencies"] = getActiveCurrencies();
  root["availableCurrencies"] = getAvailableCurrencies();

  std::vector<ScreenMapping> screenNameMap = getScreenNameMap();

  for (int i = 0; i < screenNameMap.size(); i++)
  {
    JsonObject o = screens.add<JsonObject>();
    String key = "screen" + String(screenNameMap.at(i).value) + "Visible";
    o["id"] = screenNameMap.at(i).value;
    o["name"] = String(screenNameMap.at(i).name);
    o["enabled"] = preferences.getBool(key.c_str(), true);
  }

  root["poolLogosUrl"] = preferences.getString("poolLogosUrl", DEFAULT_MINING_POOL_LOGOS_URL);
  root["ceEndpoint"] = preferences.getString("ceEndpoint", DEFAULT_CUSTOM_ENDPOINT);
  root["ceDisableSSL"] = preferences.getBool("ceDisableSSL", DEFAULT_CUSTOM_ENDPOINT_DISABLE_SSL);

  // Add DND settings
  auto& ledHandler = getLedHandler();
  root["dnd"]["enabled"] = ledHandler.isDNDEnabled();
  root["dnd"]["timeBasedEnabled"] = ledHandler.isDNDTimeBasedEnabled();
  root["dnd"]["startHour"] = ledHandler.getDNDStartHour();
  root["dnd"]["startMinute"] = ledHandler.getDNDStartMinute();
  root["dnd"]["endHour"] = ledHandler.getDNDEndHour();
  root["dnd"]["endMinute"] = ledHandler.getDNDEndMinute();

  AsyncResponseStream *response =
      request->beginResponseStream(JSON_CONTENT);
  serializeJson(root, *response);

  request->send(response);
}

bool processEpdColorSettings(AsyncWebServerRequest *request)
{
  bool settingsChanged = false;
  if (request->hasParam("fgColor", true))
  {
    const AsyncWebParameter *fgColor = request->getParam("fgColor", true);
    uint32_t color = strtol(fgColor->value().c_str(), NULL, 16);
    preferences.putUInt("fgColor", color);
    setFgColor(color);
    // Serial.print(F("Setting foreground color to "));
    // Serial.println(fgColor->value().c_str());
    settingsChanged = true;
  }
  if (request->hasParam("bgColor", true))
  {
    const AsyncWebParameter *bgColor = request->getParam("bgColor", true);

    uint32_t color = strtol(bgColor->value().c_str(), NULL, 16);
    preferences.putUInt("bgColor", color);
    setBgColor(color);
    // Serial.print(F("Setting background color to "));
    // Serial.println(bgColor->value().c_str());
    settingsChanged = true;
  }

  return settingsChanged;
}

void onApiSystemStatus(AsyncWebServerRequest *request)
{
  AsyncResponseStream *response =
      request->beginResponseStream(JSON_CONTENT);

  JsonDocument root;

  root["espFreeHeap"] = ESP.getFreeHeap();
  root["espHeapSize"] = ESP.getHeapSize();
  root["espFreePsram"] = ESP.getFreePsram();
  root["espPsramSize"] = ESP.getPsramSize();
  root["fsUsedBytes"] = LittleFS.usedBytes();
  root["fsTotalBytes"] = LittleFS.totalBytes();

  root["rssi"] = WiFi.RSSI();
  root["txPower"] = WiFi.getTxPower();

  serializeJson(root, *response);

  request->send(response);
}

#define STRINGIFY(x) #x
#define ENUM_TO_STRING(x) STRINGIFY(x)

void onApiSetWifiTxPower(AsyncWebServerRequest *request)
{
  if (request->hasParam("txPower"))
  {
    const AsyncWebParameter *txPowerParam = request->getParam("txPower");
    int txPower = txPowerParam->value().toInt();
    if (static_cast<int>(wifi_power_t::WIFI_POWER_MINUS_1dBm) <= txPower &&
        txPower <= static_cast<int>(wifi_power_t::WIFI_POWER_19_5dBm))
    {
      // is valid value
      String txPowerName =
          std::to_string(
              static_cast<std::underlying_type_t<wifi_power_t>>(txPower))
              .c_str();

      Serial.printf("Set WiFi Tx power to: %s\n", txPowerName);

      if (WiFi.setTxPower(static_cast<wifi_power_t>(txPower)))
      {
        preferences.putInt("txPower", txPower);
        request->send(HTTP_OK, "application/json", "{\"setTxPower\": \"ok\"}");
        return;
      }
    }
  }

  return request->send(HTTP_BAD_REQUEST);
}

void onApiLightsStatus(AsyncWebServerRequest *request)
{
  AsyncResponseStream *response =
      request->beginResponseStream(JSON_CONTENT);

  serializeJson(getLedStatusObject()["data"], *response);

  request->send(response);
}

void onApiStopDataSources(AsyncWebServerRequest *request)
{
  AsyncResponseStream *response =
      request->beginResponseStream(JSON_CONTENT);

  stopPriceNotify();
  stopBlockNotify();

  request->send(response);
}

void onApiRestartDataSources(AsyncWebServerRequest *request)
{
  AsyncResponseStream *response =
      request->beginResponseStream(JSON_CONTENT);

  restartPriceNotify();
  restartBlockNotify();
  //  setupPriceNotify();
  //  setupBlockNotify();

  request->send(response);
}

void onApiLightsOff(AsyncWebServerRequest *request)
{
  auto& ledHandler = getLedHandler();
  ledHandler.setLights(0, 0, 0);
  request->send(HTTP_OK);
}

void onApiLightsSetColor(AsyncWebServerRequest *request)
{
  if (request->hasParam("c"))
  {
    AsyncResponseStream *response =
        request->beginResponseStream(JSON_CONTENT);

    String rgbColor = request->getParam("c")->value();

    if (rgbColor.compareTo("off") == 0)
    {
      auto& ledHandler = getLedHandler();
      ledHandler.setLights(0, 0, 0);
    }
    else
    {
      uint r, g, b;
      sscanf(rgbColor.c_str(), "%02x%02x%02x", &r, &g, &b);
      auto& ledHandler = getLedHandler();
      ledHandler.setLights(r, g, b);
    }

    JsonDocument doc;
    doc["result"] = rgbColor;

    serializeJson(getLedStatusObject()["data"], *response);

    request->send(response);
  }
  else
  {
    request->send(HTTP_BAD_REQUEST);
  }
}

void onApiLightsSetJson(AsyncWebServerRequest *request, JsonVariant &json)
{
  auto& ledHandler = getLedHandler();
  auto& pixels = ledHandler.getPixels();
  
  JsonArray lights = json.as<JsonArray>();

  if (lights.size() != pixels.numPixels())
  {
    if (!lights.size())
    {
      // if empty, assume off request
      return onApiLightsOff(request);
    }

    Serial.printf("Invalid values for LED set %d\n", lights.size());
    request->send(HTTP_BAD_REQUEST);
    return;
  }

  for (uint i = 0; i < pixels.numPixels(); i++)
  {
    unsigned int red, green, blue;

    if (lights[i]["red"].is<uint>() && lights[i]["green"].is<uint>() &&
        lights[i]["blue"].is<uint>())
    {
      red = lights[i]["red"].as<uint>();
      green = lights[i]["green"].as<uint>();
      blue = lights[i]["blue"].as<uint>();
    }
    else if (lights[i]["hex"].is<const char*>())
    {
      if (!sscanf(lights[i]["hex"].as<String>().c_str(), "#%02X%02X%02X", &red,
                  &green, &blue) == 3)
      {
        Serial.printf("Invalid hex for LED %d\n", i);
        request->send(HTTP_BAD_REQUEST);
        return;
      }
    }
    else
    {
      Serial.printf("No valid color for LED %d\n", i);
      request->send(HTTP_BAD_REQUEST);
      return;
    }

    pixels.setPixelColor((pixels.numPixels() - i - 1),
                         pixels.Color(red, green, blue));
  }

  pixels.show();
  ledHandler.saveLedState();

  request->send(HTTP_OK);
}

void onIndex(AsyncWebServerRequest *request)
{
  request->send(LittleFS, "/index.html", String(), false);
}

void onNotFound(AsyncWebServerRequest *request)
{
   // Access-Control-Request-Method == POST might be better

  if (request->method() == HTTP_OPTIONS ||
      request->hasHeader("Sec-Fetch-Mode"))
  {
    // Serial.printf("NotFound, Return[%d]\n", 200);

    request->send(HTTP_OK);
  }
  else
  {
    // Serial.printf("NotFound, Return[%d]\n", 404);
    request->send(404);
  }
};

void eventSourceTask(void *pvParameters)
{
  for (;;)
  {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    eventSourceUpdate();
  }
}

void onApiShowCurrency(AsyncWebServerRequest *request)
{
  if (request->hasParam("c"))
  {
    const AsyncWebParameter *p = request->getParam("c");
    std::string currency = p->value().c_str();

    if (!isActiveCurrency(currency))
    {
      request->send(404);
      return;
    }

    char curChar = getCurrencyChar(currency);

    ScreenHandler::setCurrentCurrency(curChar);
    ScreenHandler::setCurrentScreen(ScreenHandler::getCurrentScreen());

    request->send(HTTP_OK);
    return;
  }
  request->send(404);
}

#ifdef HAS_FRONTLIGHT
void onApiFrontlightOn(AsyncWebServerRequest *request)
{
  auto& ledHandler = getLedHandler();
  ledHandler.frontlightFadeInAll();

  request->send(HTTP_OK);
}

void onApiFrontlightStatus(AsyncWebServerRequest *request)
{
  auto& ledHandler = getLedHandler();
  AsyncResponseStream *response =
      request->beginResponseStream(JSON_CONTENT);

  JsonDocument root;

  std::vector<uint16_t> statuses = ledHandler.frontlightGetStatus();
  uint16_t arr[NUM_SCREENS];
  std::copy(statuses.begin(), statuses.end(), arr);

  JsonArray data = root["flStatus"].to<JsonArray>();
  copyArray(arr, data);
  serializeJson(root, *response);

  request->send(response);
}

void onApiFrontlightFlash(AsyncWebServerRequest *request)
{
  auto& ledHandler = getLedHandler();
  ledHandler.frontlightFlash(preferences.getUInt("flEffectDelay"));

  request->send(HTTP_OK);
}

void onApiFrontlightSetBrightness(AsyncWebServerRequest *request)
{
  if (request->hasParam("b"))
  {
    auto& ledHandler = getLedHandler();
    ledHandler.frontlightSetBrightness(request->getParam("b")->value().toInt());
    request->send(HTTP_OK);
  }
  else
  {
    request->send(HTTP_BAD_REQUEST);
  }
}

void onApiFrontlightOff(AsyncWebServerRequest *request)
{
  auto& ledHandler = getLedHandler();
  ledHandler.frontlightFadeOutAll();

  request->send(HTTP_OK);
}
#endif

void onApiDNDTimeBasedEnable(AsyncWebServerRequest *request) {
  auto& ledHandler = getLedHandler();
  ledHandler.setDNDTimeBasedEnabled(true);
  request->send(200);
}

void onApiDNDTimeBasedDisable(AsyncWebServerRequest *request) {
  auto& ledHandler = getLedHandler();
  ledHandler.setDNDTimeBasedEnabled(false);
  request->send(200);
}

void onApiDNDSetTimeRange(AsyncWebServerRequest *request) {
  if (request->hasParam("startHour") && request->hasParam("startMinute") &&
      request->hasParam("endHour") && request->hasParam("endMinute")) {
    auto& ledHandler = getLedHandler();
    uint8_t startHour = request->getParam("startHour")->value().toInt();
    uint8_t startMinute = request->getParam("startMinute")->value().toInt();
    uint8_t endHour = request->getParam("endHour")->value().toInt();
    uint8_t endMinute = request->getParam("endMinute")->value().toInt();
    
    ledHandler.setDNDTimeRange(startHour, startMinute, endHour, endMinute);
    request->send(200);
  } else {
    request->send(400);
  }
}

void onApiDNDStatus(AsyncWebServerRequest *request) {
  auto& ledHandler = getLedHandler();
  JsonDocument doc;
  doc["enabled"] = ledHandler.isDNDEnabled();
  doc["timeBasedEnabled"] = ledHandler.isDNDTimeBasedEnabled();
  doc["startTime"] = String(ledHandler.getDNDStartHour()) + ":" + 
                     (ledHandler.getDNDStartMinute() < 10 ? "0" : "") + String(ledHandler.getDNDStartMinute());
  doc["endTime"] = String(ledHandler.getDNDEndHour()) + ":" + 
                 (ledHandler.getDNDEndMinute() < 10 ? "0" : "") + String(ledHandler.getDNDEndMinute());
  doc["active"] = ledHandler.isDNDActive();
  
  String response;
  serializeJson(doc, response);
  request->send(200, "application/json", response);
}

void onApiDNDEnable(AsyncWebServerRequest *request) {
  auto& ledHandler = getLedHandler();
  ledHandler.setDNDEnabled(true);
  request->send(200);
}

void onApiDNDDisable(AsyncWebServerRequest *request) {
  auto& ledHandler = getLedHandler();
  ledHandler.setDNDEnabled(false);
  request->send(200);
}

void onApiLightsGet(AsyncWebServerRequest *request)
{
  auto& ledHandler = getLedHandler();
  auto& pixels = ledHandler.getPixels();
  
  DynamicJsonDocument doc(1024);
  JsonArray lights = doc.createNestedArray("lights");

  for (uint i = 0; i < pixels.numPixels(); i++)
  {
    uint32_t pixColor = pixels.getPixelColor(pixels.numPixels() - i - 1);
    JsonObject light = lights.createNestedObject();
    light["r"] = (uint8_t)(pixColor >> 16);
    light["g"] = (uint8_t)(pixColor >> 8);
    light["b"] = (uint8_t)pixColor;
  }

  String output;
  serializeJson(doc, output);
  request->send(200, "application/json", output);
}

void onApiLightsPost(AsyncWebServerRequest *request, uint8_t *data, size_t len,
                    size_t index, size_t total)
{
  auto& ledHandler = getLedHandler();
  auto& pixels = ledHandler.getPixels();
  
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, data);
  if (error)
  {
    request->send(400);
    return;
  }

  JsonArray lights = doc["lights"];
  if (lights.size() != pixels.numPixels())
  {
    request->send(400);
    return;
  }

  for (uint i = 0; i < pixels.numPixels(); i++)
  {
    JsonObject light = lights[i];
    uint8_t red = light["r"];
    uint8_t green = light["g"];
    uint8_t blue = light["b"];

    pixels.setPixelColor((pixels.numPixels() - i - 1),
                        pixels.Color(red, green, blue));
  }
  pixels.show();

  request->send(200);
}

void onApiSettings(AsyncWebServerRequest *request, JsonVariant &json)
{
  JsonObject settings = json.as<JsonObject>();
  auto& ledHandler = getLedHandler();

  if (settings.containsKey("dnd")) {
    JsonObject dndObj = settings["dnd"];
    if (dndObj.containsKey("timeBasedEnabled")) {
      ledHandler.setDNDTimeBasedEnabled(dndObj["timeBasedEnabled"].as<bool>());
    }
    if (dndObj.containsKey("startHour") && dndObj.containsKey("startMinute") &&
        dndObj.containsKey("endHour") && dndObj.containsKey("endMinute")) {
      ledHandler.setDNDTimeRange(
          dndObj["startHour"].as<uint8_t>(),
          dndObj["startMinute"].as<uint8_t>(),
          dndObj["endHour"].as<uint8_t>(),
          dndObj["endMinute"].as<uint8_t>());
    }
  }
}