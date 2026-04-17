#include "webserver.hpp"
#include "lib/led_handler.hpp"
#include "lib/pref_keys.hpp"
#include "lib/shared.hpp"
#include "esp_partition.h"

static const char* JSON_CONTENT = "application/json";

// The three arrays below drive the generic branch of onApiSettingsPatch and
// the schema exposed by onApiSettingsGet. Routing keys through PrefKeys
// guarantees the identifier matches the backing NVS slot and makes it easy
// to see every PATCH-accepted setting in one place.
static const char *const PROGMEM strSettings[] = {
    PrefKeys::HostnamePrefix, PrefKeys::MempoolInstance, PrefKeys::NostrPubKey,
    PrefKeys::NostrRelay, PrefKeys::BitaxeHostname, PrefKeys::MiningPoolName,
    PrefKeys::MiningPoolUser, PrefKeys::NostrZapPubkey, PrefKeys::HttpAuthUser,
    PrefKeys::HttpAuthPass, PrefKeys::GitReleaseUrl, PrefKeys::PoolLogosUrl,
    PrefKeys::CeEndpoint, PrefKeys::FontName, PrefKeys::LocalPoolHost,
    PrefKeys::TzString};

static const char *const PROGMEM uintSettings[] = {
    PrefKeys::MinSecPriceUpd, PrefKeys::FullRefreshMin, PrefKeys::LedBrightness,
    PrefKeys::FlMaxBrightness, PrefKeys::FlEffectDelay, PrefKeys::LuxLightToggle,
    PrefKeys::WpTimeout, PrefKeys::BlockFlashColor};

static const char *const PROGMEM boolSettings[] = {
    PrefKeys::LedTestOnPower, PrefKeys::LedFlashOnUpd, PrefKeys::MdnsEnabled,
    PrefKeys::OtaEnabled, PrefKeys::StealFocus, PrefKeys::McapBigChar,
    PrefKeys::UseSatsSymbol, PrefKeys::UseBlkCountdown, PrefKeys::SuffixPrice,
    PrefKeys::DisableLeds, PrefKeys::MowMode, PrefKeys::SuffixShareDot,
    PrefKeys::FlOffWhenDark, PrefKeys::FlAlwaysOn, PrefKeys::FlDisable,
    PrefKeys::FlFlashOnUpd, PrefKeys::MempoolSecure, PrefKeys::BitaxeEnabled,
    PrefKeys::MiningPoolStats, PrefKeys::VerticalDesc, PrefKeys::NostrZapNotify,
    PrefKeys::HttpAuthEnabled, PrefKeys::EnableDebugLog, PrefKeys::CeDisableSSL,
    PrefKeys::DndEnabled, PrefKeys::DndTimeEnabled, PrefKeys::ScrnRestoreZap,
    PrefKeys::BlockFeeDec, PrefKeys::SupplyPercent, PrefKeys::RefrScrnChange,
    PrefKeys::InverseButtons};

AsyncWebServer server(80);
AsyncEventSource events("/events");
TaskHandle_t eventSourceTaskHandle;

#define HTTP_OK 200
#define HTTP_BAD_REQUEST 400
#define HTTP_NOT_FOUND 404
#define HTTP_SERVICE_UNAVAILABLE 503

// Reboot from a dedicated task. Calling esp_restart() directly from an
// AsyncTCP callback (e.g. request->onDisconnect) used to be paired with
// noInterrupts(), which masked the scheduler tick; esp_wifi_stop() inside
// esp_restart() then blocked on a semaphore until the interrupt WDT fired and
// the device panicked. Running the delay + restart on a separate task keeps
// interrupts enabled and lets the AsyncTCP task finish cleanly before reboot.
static void scheduleDelayedRestart(uint32_t delayMs = 500)
{
  static volatile bool s_restartScheduled = false;
  if (s_restartScheduled) return;
  s_restartScheduled = true;

  xTaskCreate(
      [](void *arg) {
        uint32_t ms = (uint32_t)(uintptr_t)arg;
        vTaskDelay(pdMS_TO_TICKS(ms));
        esp_restart();
      },
      "restart", 2048, (void *)(uintptr_t)delayMs, tskIDLE_PRIORITY + 1,
      nullptr);
}

// Centralised HTTP auth gate used by sensitive endpoints. Returns true and
// has already sent a 401/auth prompt if the caller is not authenticated.
static bool requireHttpAuth(AsyncWebServerRequest *request)
{
  if (!preferences.getBool("httpAuthEnabled", DEFAULT_HTTP_AUTH_ENABLED)) {
    return false;
  }
  if (!request->authenticate(
          preferences.getString("httpAuthUser", DEFAULT_HTTP_AUTH_USERNAME).c_str(),
          preferences.getString("httpAuthPass", DEFAULT_HTTP_AUTH_PASSWORD).c_str()))
  {
    request->requestAuthentication();
    return true;
  }
  return false;
}

static inline void notifyEventSourceStatus()
{
  if (eventSourceTaskHandle != NULL) xTaskNotifyGive(eventSourceTaskHandle);
}

void setupWebserver()
{
  events.onConnect([](AsyncEventSourceClient *client)
                   { client->send("welcome", NULL, millis(), 1000);
                     });
  server.addHandler(&events);

  AsyncStaticWebHandler &staticHandler = server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

  server.rewrite("/convert", "/");
  server.rewrite("/api", "/");

  if (preferences.getBool("httpAuthEnabled", DEFAULT_HTTP_AUTH_ENABLED))
  {
    String authUser = preferences.getString("httpAuthUser", DEFAULT_HTTP_AUTH_USERNAME);
    String authPass = preferences.getString("httpAuthPass", DEFAULT_HTTP_AUTH_PASSWORD);
    staticHandler.setAuthentication(authUser, authPass);
    // EventSource / SSE is just a long-lived GET; when HTTP auth is on,
    // the stream leaks live device status to unauthenticated clients
    // unless we lock it too.
    events.setAuthentication(authUser.c_str(), authPass.c_str());
  }
  //  server.on("/", HTTP_GET, onIndex);
  server.on("/api/status", HTTP_GET, onApiStatus);
  server.on("/api/system_status", HTTP_GET, onApiSystemStatus);
  // State changes moved to POST so they stop showing up in browser history,
  // bookmark warmers, and <link rel="prefetch"> runs. The old GET routes are
  // intentionally not registered; the WebUI is being rebuilt against 3.4.0.
  server.on("/api/wifi_set_tx_power", HTTP_POST, onApiSetWifiTxPower);

  server.on("/api/full_refresh", HTTP_POST, onApiFullRefresh);

  server.on("/api/stop_datasources", HTTP_POST, onApiStopDataSources);
  server.on("/api/restart_datasources", HTTP_POST, onApiRestartDataSources);

  server.on("/api/action/pause", HTTP_POST, onApiActionPause);
  server.on("/api/action/timer_restart", HTTP_POST, onApiActionTimerRestart);

  server.on("/api/settings", HTTP_GET, onApiSettingsGet);

  server.on("/api/show/screen", HTTP_POST, onApiShowScreen);
  server.on("/api/show/currency", HTTP_POST, onApiShowCurrency);

  server.on("/api/show/text", HTTP_POST, onApiShowText);

  server.on("/api/screen/next", HTTP_POST, onApiScreenControl);
  server.on("/api/screen/previous", HTTP_POST, onApiScreenControl);

  // PATCH /api/settings replaces the old POST-ish /api/json/settings so the
  // settings endpoint pair (GET read / PATCH write) lives on a single URL.
  AsyncCallbackJsonWebHandler *settingsPatchHandler =
      new AsyncCallbackJsonWebHandler("/api/settings", onApiSettingsPatch);
  settingsPatchHandler->setMethod(HTTP_PATCH);
  server.addHandler(settingsPatchHandler);

  AsyncCallbackJsonWebHandler *handler = new AsyncCallbackJsonWebHandler(
      "/api/show/custom", onApiShowTextAdvanced);
  handler->setMethod(HTTP_POST);
  server.addHandler(handler);

  AsyncCallbackJsonWebHandler *lightsJsonHandler =
      new AsyncCallbackJsonWebHandler("/api/lights/set", onApiLightsSetJson);
  lightsJsonHandler->setMethod(HTTP_POST);
  server.addHandler(lightsJsonHandler);

  server.on("/api/lights/off", HTTP_POST, onApiLightsOff);
  server.on("/api/lights/color", HTTP_POST, onApiLightsSetColor);
  server.on("/api/lights", HTTP_GET, onApiLightsStatus);
  server.on("/api/identify", HTTP_POST, onApiIdentify);

#ifdef HAS_FRONTLIGHT
  server.on("/api/frontlight/on", HTTP_POST, onApiFrontlightOn);
  server.on("/api/frontlight/flash", HTTP_POST, onApiFrontlightFlash);
  server.on("/api/frontlight/status", HTTP_GET, onApiFrontlightStatus);

  server.on("/api/frontlight/brightness", HTTP_POST, onApiFrontlightSetBrightness);
  server.on("/api/frontlight/off", HTTP_POST, onApiFrontlightOff);

  server.addRewrite(
      new OneParamRewrite("/api/frontlight/brightness/{b}", "/api/frontlight/brightness?b={b}"));
#endif

  // server.on("^\\/api\\/lights\\/([A-Fa-f0-9]{6}|[A-Fa-f0-9]{3})$", HTTP_POST,
  // onApiLightsSetColor);

  if (preferences.getBool("otaEnabled", DEFAULT_OTA_ENABLED))
  {
    server.on("/upload/firmware", HTTP_POST, onFirmwareUpdate, asyncFirmwareUpdateHandler);
    server.on("/upload/webui", HTTP_POST, onFirmwareUpdate, asyncWebuiUpdateHandler);
    server.on("/api/firmware/auto_update", HTTP_POST, onAutoUpdateFirmware);
  }

  server.on("/api/restart", HTTP_POST, onApiRestart);
  server.addRewrite(
      new OneParamRewrite("/api/show/currency/{c}", "/api/show/currency?c={c}"));
  server.addRewrite(new OneParamRewrite("/api/lights/color/{color}",
                                        "/api/lights/color?c={color}"));
  server.addRewrite(
      new OneParamRewrite("/api/show/screen/{s}", "/api/show/screen?s={s}"));
  server.addRewrite(
      new OneParamRewrite("/api/show/text/{text}", "/api/show/text?t={text}"));
  // Placeholder name in the rewrite target must match the pattern token.
  // Previously the template used {text} so the rewrite produced a literal
  // "?t={text}" URL instead of substituting the captured number.
  server.addRewrite(new OneParamRewrite("/api/show/number/{number}",
                                        "/api/show/text?t={number}"));

  server.on("/api/dnd/status", HTTP_GET, onApiDNDStatus);
  server.on("/api/dnd/enable", HTTP_POST, onApiDNDEnable);
  server.on("/api/dnd/disable", HTTP_POST, onApiDNDDisable);

  server.onNotFound(onNotFound);

  // CORS:
  // - For normal operation, the UI is served from the device itself.
  // - During WebUI development, the UI is often served from a local dev server
  //   (e.g. http://localhost:*), which must be able to call the device API.
  //
  // Because DefaultHeaders are global (not per-request), we can't dynamically
  // echo back the request Origin here. Use a permissive "*" but keep headers
  // restricted and rely on HTTP auth for state-changing endpoints.
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods",
                                       "GET, PATCH, POST, OPTIONS");
  // Allow only the headers we actually need. "*" lets any client inject
  // arbitrary headers including Authorization, which combined with a permissive
  // Allow-Origin used to enable trivial CSRF.
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers",
                                       "Content-Type, Authorization");

  server.begin();

  if (preferences.getBool("mdnsEnabled", DEFAULT_MDNS_ENABLED))
  {
    // Must not hang the whole device if mDNS fails to start (see prior bug);
    // one attempt, log on failure, continue.
    if (MDNS.begin(getMyHostname()))
    {
      MDNS.addService("http", "tcp", 80);
      MDNS.addServiceTxt("http", "tcp", "model", "BTClock");
      MDNS.addServiceTxt("http", "tcp", "version", "3.0");
      MDNS.addServiceTxt("http", "tcp", "rev", GIT_REV);
      MDNS.addServiceTxt("http", "tcp", "hw_rev", getHwRev());
    }
    else
    {
    }
  }

  xTaskCreate(eventSourceTask, "eventSourceTask", 4096, NULL, tskIDLE_PRIORITY,
              &eventSourceTaskHandle);
}

void stopWebServer() { server.end(); }

void onFirmwareUpdate(AsyncWebServerRequest *request)
{
  // The upload body handler already bails out early if auth fails, but the
  // final response handler is called independently, so re-check here.
  if (requireHttpAuth(request)) return;

  const bool shouldReboot = !Update.hasError();
  if (shouldReboot)
  {
    // Reboot after the response is flushed to the client. Schedule via a
    // dedicated task so we don't stall the AsyncTCP callback (which previously
    // called noInterrupts()+esp_restart() and deadlocked esp_wifi_stop()).
    request->onDisconnect([]() { scheduleDelayedRestart(500); });

    if (events.count())
      events.send("closing");
  }

  AsyncWebServerResponse *response =
      request->beginResponse(200, "text/plain", shouldReboot ? "OK" : "FAIL");
  response->addHeader("Connection", "close");
  request->send(response);
}

void onAutoUpdateFirmware(AsyncWebServerRequest *request)
{
  if (requireHttpAuth(request)) return;
  UpdateMessage msg = {UPDATE_ALL};
  if (xQueueSend(otaQueue, &msg, 0) == pdTRUE)
  {
    request->send(HTTP_OK, "application/json", "{\"msg\":\"Firmware update triggered\"}");
  }
  else
  {
    request->send(HTTP_SERVICE_UNAVAILABLE, "application/json", "{\"msg\":\"Update already in progress\"}");
  }
}

void asyncWebuiUpdateHandler(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
  if (index == 0 && requireHttpAuth(request)) return;
  asyncFileUpdateHandler(request, filename, index, data, len, final, U_SPIFFS);
}

void asyncFileUpdateHandler(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final, int command)
{
  // The LittleFS partition size on some boards (e.g. lolin_s3_mini: 0x66C00)
  // is not a multiple of SPI_FLASH_SEC_SIZE. Update.begin() accepts a smaller
  // size, but Update.write() aborts with "Not Enough Space" the moment the
  // cumulative byte count exceeds that size. We therefore track the capped
  // size per upload and only hand the library bytes it will accept; the
  // remaining trailing bytes are always 0xFF padding in the generated image
  // and are ignored.
  static size_t s_fsCapSize = 0;
  static size_t s_fsWritten = 0;

  if (!index)
  {

    if (command == U_FLASH)
    {
      // Update.runAsync(true);
      // The original expression had the closing paren misplaced, so the
      // command argument was actually consumed by the comma operator and
      // Update.begin() was called with only the size. Put command back
      // inside the Update.begin() call.
      if (!Update.begin(((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000), command))
      {
        Update.printError(Serial);
        return;
      }
    }
    else if (command == U_SPIFFS)
    {
      // Unmount LittleFS so the flash driver has exclusive access to the
      // partition for the OTA erase/write cycle.
      LittleFS.end();

      // The Arduino Update library asks esp_partition_erase_range() to erase
      // a full SPI_FLASH_SEC_SIZE (4KB) sector at the tail of the partition.
      // If partition->size is not a multiple of 4KB (e.g. 0x66C00 on the
      // lolin_s3_mini 4MB partition table), that erase overruns the partition
      // and IDF returns ESP_ERR_INVALID_SIZE; Update then aborts the whole
      // upload with "Flash Erase Failed" (boards with 4KB-aligned sizes such
      // as btclock_rev_b/0xCD000 or btclock_v8/0x200000 are unaffected).
      // The last sub-sector of the littlefs image is always 0xFF padding, so
      // rounding the update size down to the sector boundary is safe.
      size_t fsSize = UPDATE_SIZE_UNKNOWN;
      const esp_partition_t *fsPart = esp_partition_find_first(
          ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, NULL);
      if (fsPart) {
        fsSize = fsPart->size & ~(SPI_FLASH_SEC_SIZE - 1);
      }
      s_fsCapSize = fsSize;
      s_fsWritten = 0;

      if (!Update.begin(fsSize, U_SPIFFS))
      {
        Update.printError(Serial);
        return;
      }
    }
  }
  if (!Update.hasError())
  {
    size_t toWrite = len;
    if (command == U_SPIFFS && s_fsCapSize > 0)
    {
      // Only hand Update.write() bytes that still fit in the capped size;
      // the remaining trailing bytes (0xFF padding in the image) are
      // silently discarded so we don't trip UPDATE_ERROR_SPACE.
      size_t remaining = (s_fsWritten >= s_fsCapSize) ? 0 : (s_fsCapSize - s_fsWritten);
      if (toWrite > remaining) toWrite = remaining;
      s_fsWritten += len;
    }
    if (toWrite > 0 && Update.write(data, toWrite) != toWrite)
    {
      Update.printError(Serial);
    }
  }
  if (final)
  {
    if (Update.end(true))
    {
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
  if (index == 0 && requireHttpAuth(request)) return;
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
  auto& blockNotify = BlockNotify::getInstance();
  conStatus["blocks"] = blockNotify.isConnected();
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
  root["dnd"]["dndTimeEnabled"] = ledHandler.isDNDTimeBasedEnabled();
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
    // colors.add(hexColor);

    JsonObject object = colors.add<JsonObject>();
    object["red"] = red;
    object["green"] = green;
    object["blue"] = blue;
    object["hex"] = hexColor;
  }

  return root;
}

void eventSourceUpdate() {
    if (!events.count()) return;
    
    static JsonDocument doc;
    doc.clear();
    
    JsonDocument root = getStatusObject();
    
    root["leds"] = getLedStatusObject()["data"];

    // Get current EPD content directly as array
    std::array<String, NUM_SCREENS> epdContent = EPDManager::getInstance().getCurrentContent();
    
    // Add EPD content arrays
    JsonArray data = root["data"].to<JsonArray>();
    
    // Copy array elements directly
    for(const auto& content : epdContent) {
      data.add(content);
    }

    String buffer;
    serializeJson(root, buffer);
    events.send(buffer.c_str(), "status");
}

/**
 * @Api
 * @Path("/api/status")
 */
void onApiStatus(AsyncWebServerRequest *request)
{
  if (requireHttpAuth(request)) return;
  AsyncResponseStream *response =
      request->beginResponseStream(JSON_CONTENT);

  JsonDocument root = getStatusObject();
  
  // Get current EPD content directly as array
  std::array<String, NUM_SCREENS> epdContent = EPDManager::getInstance().getCurrentContent();
  
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
  if (requireHttpAuth(request)) return;
  setTimerActive(false);
  request->send(HTTP_OK);
  notifyEventSourceStatus();
};

/**
 * @Api
 * @Path("/api/action/timer_restart")
 */
void onApiActionTimerRestart(AsyncWebServerRequest *request)
{
  if (requireHttpAuth(request)) return;
  setTimerActive(true);
  request->send(HTTP_OK);
  notifyEventSourceStatus();
}

/**
 * @Api
 * @Path("/api/full_refresh")
 */
void onApiFullRefresh(AsyncWebServerRequest *request)
{
  if (requireHttpAuth(request)) return;
  EPDManager::getInstance().forceFullRefresh();
  std::array<String, NUM_SCREENS> newEpdContent = EPDManager::getInstance().getCurrentContent();
  EPDManager::getInstance().setContent(newEpdContent, true);
  request->send(HTTP_OK);
  notifyEventSourceStatus();
}

/**
 * @Api
 * @Path("/api/show/screen")
 */
void onApiShowScreen(AsyncWebServerRequest *request)
{
  if (requireHttpAuth(request)) return;
  if (request->hasParam("s"))
  {
    const AsyncWebParameter *p = request->getParam("s");
    uint currentScreen = p->value().toInt();
    ScreenHandler::setCurrentScreen(currentScreen);
  }
  request->send(HTTP_OK);
  notifyEventSourceStatus();
}

/**
 * @Api
 * @Path("/api/screen/next")
 */
void onApiScreenControl(AsyncWebServerRequest *request) {
    if (requireHttpAuth(request)) return;
    const String& action = request->url();
    if (action.endsWith("/next")) {
        ScreenHandler::nextScreen();
    } else if (action.endsWith("/previous")) {
        ScreenHandler::previousScreen();
    }
    request->send(HTTP_OK);
    notifyEventSourceStatus();
}

void onApiShowText(AsyncWebServerRequest *request)
{
  if (requireHttpAuth(request)) return;
  if (request->hasParam("t"))
  {
    const AsyncWebParameter *p = request->getParam("t");
    String t = p->value();
    t.toUpperCase(); // This is needed as long as lowercase letters are glitchy

    // Clamp to t.length() so we don't read past the String when the caller
    // provided fewer than NUM_SCREENS characters (default-constructed Strings
    // for the remaining slots are empty).
    std::array<String, NUM_SCREENS> textEpdContent;
    size_t tLen = t.length();
    if (tLen > NUM_SCREENS) tLen = NUM_SCREENS;
    for (size_t i = 0; i < tLen; i++) textEpdContent[i] = t[i];

    EPDManager::getInstance().setContent(textEpdContent);
  }
  ScreenHandler::setCurrentScreen(SCREEN_CUSTOM);
  request->send(HTTP_OK);
  notifyEventSourceStatus();
}

void onApiShowTextAdvanced(AsyncWebServerRequest *request, JsonVariant &json)
{
  if (requireHttpAuth(request)) return;
  JsonArray screens = json.as<JsonArray>();

  std::array<String, NUM_SCREENS> epdContent;
  // Only take up to NUM_SCREENS entries; the previous code wrote past the
  // array end if the client sent more than NUM_SCREENS screens.
  int i = 0;
  for (JsonVariant s : screens)
  {
    if (i >= static_cast<int>(NUM_SCREENS)) break;
    epdContent[i] = s.as<String>();
    i++;
  }

  EPDManager::getInstance().setContent(epdContent);

  ScreenHandler::setCurrentScreen(SCREEN_CUSTOM);
  request->send(HTTP_OK);
  notifyEventSourceStatus();
}

void onApiSettingsPatch(AsyncWebServerRequest *request, JsonVariant &json)
{
  if (requireHttpAuth(request)) return;

  JsonObject settings = json.as<JsonObject>();

  bool settingsChanged = true;

  if (settings["invertedColor"].is<bool>())
  {
    bool inverted = settings["invertedColor"].as<bool>();
    preferences.putBool("invertedColor", inverted);
    if (inverted) {
      preferences.putUInt("fgColor", GxEPD_WHITE);
      preferences.putUInt("bgColor", GxEPD_BLACK);
      EPDManager::getInstance().setForegroundColor(GxEPD_WHITE);
      EPDManager::getInstance().setBackgroundColor(GxEPD_BLACK);
    } else {
      preferences.putUInt("fgColor", GxEPD_BLACK);
      preferences.putUInt("bgColor", GxEPD_WHITE);
      EPDManager::getInstance().setForegroundColor(GxEPD_BLACK);
      EPDManager::getInstance().setBackgroundColor(GxEPD_WHITE);
    }
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
      Serial.printf("set %s=%s\r\n", setting.c_str(),
                    settings[setting].as<String>().c_str());
    }
  }

  for (String setting : uintSettings)
  {
    if (settings[setting].is<uint>())
    {
      preferences.putUInt(setting.c_str(), settings[setting].as<uint>());
      Serial.printf("set %s=%u\r\n", setting.c_str(),
                    settings[setting].as<uint>());
    }
  }

  if (settings["tzOffset"].is<int>())
  {
    int gmtOffset = settings["tzOffset"].as<int>() * 60;
    preferences.putInt("gmtOffset", gmtOffset);
  }

  for (String setting : boolSettings)
  {
    if (settings[setting].is<bool>())
    {
      bool value = settings[setting].as<bool>();
      // DND bools need to go through LedHandler so the in-memory state used
      // by isDNDActive() stays in sync with NVS. The generic putBool path
      // below only updated NVS, which left the handler running effects
      // until the next reboot.
      if (setting == "dndEnabled")
      {
        getLedHandler().setDNDEnabled(value);
      }
      else if (setting == "dndTimeEnabled")
      {
        getLedHandler().setDNDTimeBasedEnabled(value);
      }
      else
      {
        preferences.putBool(setting.c_str(), value);
      }
      Serial.printf("set %s=%d\r\n", setting.c_str(), value);
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
      settingsChanged = true;
    }
  }

  if (settings["ceEndpoint"].is<String>()) {
    preferences.putString("ceEndpoint", settings["ceEndpoint"].as<String>());
    settingsChanged = true;
  }

  // Handle DND settings
  if (settings["dnd"].is<JsonObject>()) {
    JsonObject dndObj = settings["dnd"];
    auto& ledHandler = getLedHandler();
    
    if (dndObj["dndTimeEnabled"].is<bool>()) {
      ledHandler.setDNDTimeBasedEnabled(dndObj["dndTimeEnabled"].as<bool>());
    }
    if (dndObj["startHour"].is<uint8_t>() && dndObj["startMinute"].is<uint8_t>() &&
        dndObj["endHour"].is<uint8_t>() && dndObj["endMinute"].is<uint8_t>()) {
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
  notifyEventSourceStatus();
}

void onApiRestart(AsyncWebServerRequest *request)
{
  if (requireHttpAuth(request)) return;
  // Restart from a dedicated task so the AsyncTCP callback returns and the
  // response can be flushed before esp_restart() tears down wifi.
  request->onDisconnect([]() { scheduleDelayedRestart(500); });

  request->send(HTTP_OK);

  if (events.count())
    events.send("closing");
}

void onApiIdentify(AsyncWebServerRequest *request)
{
  if (requireHttpAuth(request)) return;
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
  if (requireHttpAuth(request)) return;

  JsonDocument root;
  root["numScreens"] = NUM_SCREENS;
  root["invertedColor"] = preferences.getBool("invertedColor", EPDManager::getInstance().getForegroundColor() == GxEPD_WHITE);
  root["timerSeconds"] = getTimerSeconds();
  root["timerRunning"] = isTimerActive();
  root["minSecPriceUpd"] = preferences.getUInt(
      "minSecPriceUpd", DEFAULT_SECONDS_BETWEEN_PRICE_UPDATE);
  root["fullRefreshMin"] =
      preferences.getUInt("fullRefreshMin", DEFAULT_MINUTES_FULL_REFRESH);
  root["wpTimeout"] = preferences.getUInt("wpTimeout", DEFAULT_WP_TIMEOUT);
  //root["tzOffset"] = preferences.getInt("gmtOffset", DEFAULT_TIME_OFFSET_SECONDS) / 60;
  root["tzString"] = preferences.getString("tzString", DEFAULT_TZ_STRING);

  // Add data source settings
  root["dataSource"] = preferences.getUChar("dataSource", DEFAULT_DATA_SOURCE);
  
  // Mempool settings (only used for THIRD_PARTY_SOURCE)
  root["mempoolInstance"] = preferences.getString("mempoolInstance", DEFAULT_MEMPOOL_INSTANCE);
  root["mempoolSecure"] = preferences.getBool("mempoolSecure", DEFAULT_MEMPOOL_SECURE);
  
  // Local pool settings
  root["localPoolHost"] = preferences.getString("localPoolHost", DEFAULT_LOCAL_POOL_ENDPOINT);
  
  // Nostr settings (used for NOSTR_SOURCE or when zapNotify is enabled)
  root["nostrPubKey"] = preferences.getString("nostrPubKey", DEFAULT_NOSTR_NPUB);
  root["nostrRelay"] = preferences.getString("nostrRelay", DEFAULT_NOSTR_RELAY);
  root["nostrZapNotify"] = preferences.getBool("nostrZapNotify", DEFAULT_ZAP_NOTIFY_ENABLED);
  root["nostrZapPubkey"] = preferences.getString("nostrZapPubkey", DEFAULT_ZAP_NOTIFY_PUBKEY);
  root["ledFlashOnZap"] = preferences.getBool("ledFlashOnZap", DEFAULT_LED_FLASH_ON_ZAP);
  root["scrnRestoreZap"] = preferences.getBool("scrnRestoreZap", DEFAULT_SCREEN_RESTORE_AFTER_ZAP);
  root["fontName"] = preferences.getString("fontName", DEFAULT_FONT_NAME);
  root["availableFonts"] = FontNames::getAvailableFonts();

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
  root["blockFeeDec"] = preferences.getBool("blockFeeDec", DEFAULT_BLOCK_FEE_DECIMALS);
  root["blockFlashColor"] = preferences.getUInt("blockFlashColor", DEFAULT_BLOCK_FLASH_COLOR);
  root["supplyPercent"] = preferences.getBool("supplyPercent", DEFAULT_SUPPLY_PERCENT);
  root["refrScrnChange"] = preferences.getBool("refrScrnChange", DEFAULT_REFRESH_ON_SCREEN_CHANGE);
  root["inverseButtons"] = preferences.getBool("inverseButtons", DEFAULT_INVERSE_BUTTONS);
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
  // Never ship the raw password to the client. Expose a boolean flag instead
  // so the UI can show "password set" without giving out credentials.
  root["httpAuthPassSet"] =
      preferences.getString("httpAuthPass", DEFAULT_HTTP_AUTH_PASSWORD).length() > 0;
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
  root["dnd"]["dndTimeEnabled"] = ledHandler.isDNDTimeBasedEnabled();
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
    EPDManager::getInstance().setForegroundColor(color);
    // Serial.print(F("Setting foreground color to "));
    // Serial.println(fgColor->value().c_str());
    settingsChanged = true;
  }
  if (request->hasParam("bgColor", true))
  {
    const AsyncWebParameter *bgColor = request->getParam("bgColor", true);

    uint32_t color = strtol(bgColor->value().c_str(), NULL, 16);
    preferences.putUInt("bgColor", color);
    EPDManager::getInstance().setBackgroundColor(color);
    // Serial.print(F("Setting background color to "));
    // Serial.println(bgColor->value().c_str());
    settingsChanged = true;
  }

  return settingsChanged;
}

void onApiSystemStatus(AsyncWebServerRequest *request)
{
  if (requireHttpAuth(request)) return;
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
  if (requireHttpAuth(request)) return;
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
  if (requireHttpAuth(request)) return;
  AsyncResponseStream *response =
      request->beginResponseStream(JSON_CONTENT);

  serializeJson(getLedStatusObject()["data"], *response);

  request->send(response);
}

void onApiStopDataSources(AsyncWebServerRequest *request)
{
  if (requireHttpAuth(request)) return;
  AsyncResponseStream *response =
      request->beginResponseStream(JSON_CONTENT);

  stopPriceNotify();
  BlockNotify::getInstance().stop();

  request->send(response);
  notifyEventSourceStatus();
}

void onApiRestartDataSources(AsyncWebServerRequest *request)
{
  if (requireHttpAuth(request)) return;
  AsyncResponseStream *response =
      request->beginResponseStream(JSON_CONTENT);

  if (getDataSource() == BTCLOCK_SOURCE || getDataSource() == CUSTOM_SOURCE)
  {
    V2Notify::restartV2Notify();
  }
  else if (getDataSource() == THIRD_PARTY_SOURCE)
  {
    BlockNotify::getInstance().restart();
    restartPriceNotify();
  }

  request->send(response);
  notifyEventSourceStatus();
}

void onApiLightsOff(AsyncWebServerRequest *request)
{
  if (requireHttpAuth(request)) return;
  auto& ledHandler = getLedHandler();
  ledHandler.setLights(0, 0, 0);
  request->send(HTTP_OK);
  notifyEventSourceStatus();
}

void onApiLightsSetColor(AsyncWebServerRequest *request)
{
  if (requireHttpAuth(request)) return;
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
      unsigned int r = 0, g = 0, b = 0;
      // Validate that we actually parsed three byte values; otherwise reject
      // the request instead of using uninitialised memory.
      if (sscanf(rgbColor.c_str(), "%2x%2x%2x", &r, &g, &b) != 3)
      {
        request->send(HTTP_BAD_REQUEST);
        return;
      }
      auto& ledHandler = getLedHandler();
      ledHandler.setLights(r, g, b);
    }

    JsonDocument doc;
    doc["result"] = rgbColor;

    serializeJson(getLedStatusObject()["data"], *response);

    request->send(response);
    notifyEventSourceStatus();
  }
  else
  {
    request->send(HTTP_BAD_REQUEST);
  }
}

void onApiLightsSetJson(AsyncWebServerRequest *request, JsonVariant &json)
{
  if (requireHttpAuth(request)) return;
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
      if (sscanf(lights[i]["hex"].as<String>().c_str(), "#%02X%02X%02X", &red,
                 &green, &blue) != 3)
      {
        request->send(HTTP_BAD_REQUEST);
        return;
      }
    }
    else
    {
      request->send(HTTP_BAD_REQUEST);
      return;
    }

    pixels.setPixelColor((pixels.numPixels() - i - 1),
                         pixels.Color(red, green, blue));
  }

  pixels.show();
  ledHandler.saveLedState();

  request->send(HTTP_OK);
  notifyEventSourceStatus();
}

void onIndex(AsyncWebServerRequest *request)
{
  request->send(LittleFS, "/index.html", String(), false);
}

void onNotFound(AsyncWebServerRequest *request)
{
  // CORS preflight: 200 with the default CORS headers is enough for the
  // browser to then make the real request.
  if (request->method() == HTTP_OPTIONS)
  {
    request->send(HTTP_OK);
    return;
  }

  // Anything else really is 404. The old Sec-Fetch-Mode heuristic returned
  // 200 for fetch/XHR requests, which masked unknown endpoints and made the
  // API look like it was succeeding.
  request->send(HTTP_NOT_FOUND);
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
  if (requireHttpAuth(request)) return;
  if (request->hasParam("c"))
  {
    const AsyncWebParameter *p = request->getParam("c");
    std::string currency = p->value().c_str();

    if (!isActiveCurrency(currency))
    {
      request->send(HTTP_NOT_FOUND);
      return;
    }

    char curChar = getCurrencyChar(currency);

    ScreenHandler::setCurrentCurrency(curChar);
    ScreenHandler::setCurrentScreen(ScreenHandler::getCurrentScreen());

    request->send(HTTP_OK);
    notifyEventSourceStatus();
    return;
  }
  request->send(HTTP_NOT_FOUND);
}

#ifdef HAS_FRONTLIGHT
void onApiFrontlightOn(AsyncWebServerRequest *request)
{
  if (requireHttpAuth(request)) return;
  auto& ledHandler = getLedHandler();
  ledHandler.frontlightFadeInAll();

  request->send(HTTP_OK);
  notifyEventSourceStatus();
}

void onApiFrontlightStatus(AsyncWebServerRequest *request)
{
  if (requireHttpAuth(request)) return;
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
  if (requireHttpAuth(request)) return;
  auto& ledHandler = getLedHandler();
  ledHandler.frontlightFlash(preferences.getUInt("flEffectDelay"));

  request->send(HTTP_OK);
  notifyEventSourceStatus();
}

void onApiFrontlightSetBrightness(AsyncWebServerRequest *request)
{
  if (requireHttpAuth(request)) return;
  if (request->hasParam("b"))
  {
    auto& ledHandler = getLedHandler();
    ledHandler.frontlightSetBrightness(request->getParam("b")->value().toInt());
    request->send(HTTP_OK);
    notifyEventSourceStatus();
  }
  else
  {
    request->send(HTTP_BAD_REQUEST);
  }
}

void onApiFrontlightOff(AsyncWebServerRequest *request)
{
  if (requireHttpAuth(request)) return;
  auto& ledHandler = getLedHandler();
  ledHandler.frontlightFadeOutAll();

  request->send(HTTP_OK);
  notifyEventSourceStatus();
}
#endif

void onApiDNDTimeBasedEnable(AsyncWebServerRequest *request) {
  if (requireHttpAuth(request)) return;
  auto& ledHandler = getLedHandler();
  ledHandler.setDNDTimeBasedEnabled(true);
  request->send(HTTP_OK);
  notifyEventSourceStatus();
}

void onApiDNDTimeBasedDisable(AsyncWebServerRequest *request) {
  if (requireHttpAuth(request)) return;
  auto& ledHandler = getLedHandler();
  ledHandler.setDNDTimeBasedEnabled(false);
  request->send(HTTP_OK);
  notifyEventSourceStatus();
}

void onApiDNDSetTimeRange(AsyncWebServerRequest *request) {
  if (requireHttpAuth(request)) return;
  if (request->hasParam("startHour") && request->hasParam("startMinute") &&
      request->hasParam("endHour") && request->hasParam("endMinute")) {
    auto& ledHandler = getLedHandler();
    uint8_t startHour = request->getParam("startHour")->value().toInt();
    uint8_t startMinute = request->getParam("startMinute")->value().toInt();
    uint8_t endHour = request->getParam("endHour")->value().toInt();
    uint8_t endMinute = request->getParam("endMinute")->value().toInt();
    
    ledHandler.setDNDTimeRange(startHour, startMinute, endHour, endMinute);
    request->send(HTTP_OK);
    notifyEventSourceStatus();
  } else {
    request->send(HTTP_BAD_REQUEST);
  }
}

void onApiDNDStatus(AsyncWebServerRequest *request) {
  if (requireHttpAuth(request)) return;
  auto& ledHandler = getLedHandler();
  JsonDocument doc;
  doc["enabled"] = ledHandler.isDNDEnabled();
  doc["dndTimeEnabled"] = ledHandler.isDNDTimeBasedEnabled();
  doc["startTime"] = String(ledHandler.getDNDStartHour()) + ":" + 
                     (ledHandler.getDNDStartMinute() < 10 ? "0" : "") + String(ledHandler.getDNDStartMinute());
  doc["endTime"] = String(ledHandler.getDNDEndHour()) + ":" + 
                 (ledHandler.getDNDEndMinute() < 10 ? "0" : "") + String(ledHandler.getDNDEndMinute());
  doc["active"] = ledHandler.isDNDActive();
  
  String response;
  serializeJson(doc, response);
  request->send(HTTP_OK, "application/json", response);
}

void onApiDNDEnable(AsyncWebServerRequest *request) {
  if (requireHttpAuth(request)) return;
  auto& ledHandler = getLedHandler();
  ledHandler.setDNDEnabled(true);
  request->send(HTTP_OK);
  notifyEventSourceStatus();
}

void onApiDNDDisable(AsyncWebServerRequest *request) {
  if (requireHttpAuth(request)) return;
  auto& ledHandler = getLedHandler();
  ledHandler.setDNDEnabled(false);
  request->send(HTTP_OK);
  notifyEventSourceStatus();
}

void onApiLightsGet(AsyncWebServerRequest *request)
{
  auto& ledHandler = getLedHandler();
  auto& pixels = ledHandler.getPixels();
  
  JsonDocument doc;
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
  request->send(HTTP_OK, "application/json", output);
}

void onApiLightsPost(AsyncWebServerRequest *request, uint8_t *data, size_t len,
                    size_t index, size_t total)
{
  if (requireHttpAuth(request)) return;
  auto& ledHandler = getLedHandler();
  auto& pixels = ledHandler.getPixels();
  
  JsonDocument doc;
  // Use the length-aware overload so the parser does not run off the end of
  // the buffer for non-NUL-terminated chunks.
  DeserializationError error = deserializeJson(doc, data, len);
  if (error)
  {
    request->send(HTTP_BAD_REQUEST);
    return;
  }

  JsonArray lights = doc["lights"];
  if (lights.size() != pixels.numPixels())
  {
    request->send(HTTP_BAD_REQUEST);
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

  request->send(HTTP_OK);
  notifyEventSourceStatus();
}