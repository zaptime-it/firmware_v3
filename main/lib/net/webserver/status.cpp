#include "internal.hpp"

#include "lib/drivers/leds/led_handler.hpp"
#include "lib/system/shared.hpp"

// buildLedStatusJson lives here (rather than in lights.cpp) because the SSE
// stream and /api/status both fold the LED array into their top-level
// response under "leds"; keeping the builder next to them avoids an include
// cycle where status.cpp would depend on lights.cpp.
JsonDocument buildLedStatusJson() {
  auto &ledHandler = getLedHandler();
  auto &pixels = ledHandler.getPixels();

  JsonDocument root;
  JsonArray colors = root["data"].to<JsonArray>();

  for (uint i = 0; i < pixels.numPixels(); i++) {
    uint32_t pixColor = pixels.getPixelColor(pixels.numPixels() - i - 1);
    uint red = (pixColor >> 16) & 0xFF;
    uint green = (pixColor >> 8) & 0xFF;
    uint blue = pixColor & 0xFF;
    char hexColor[8];
    snprintf(hexColor, sizeof(hexColor), "#%02X%02X%02X", red, green, blue);

    JsonObject object = colors.add<JsonObject>();
    object["red"] = red;
    object["green"] = green;
    object["blue"] = blue;
    object["hex"] = hexColor;
  }

  return root;
}

// The single source of truth for the status JSON that /api/status responds
// with and that the SSE stream pushes on change. Previously there were two
// copies of this logic, one in onApiStatus() and one in eventSourceUpdate(),
// which meant new status fields had to be added in both places and were
// routinely forgotten in one.
JsonDocument buildStatusJson() {
  auto &ledHandler = getLedHandler();
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
  conStatus["blocks"] = BlockNotify::getInstance().isConnected();
  conStatus["V2"] = V2Notify::isV2NotifyConnected();
  conStatus["nostr"] = nostrConnected();

  root["rssi"] = WiFi.RSSI();
  root["currency"] = getCurrencyCode(ScreenHandler::getCurrentCurrency());

#ifdef HAS_FRONTLIGHT
  std::vector<uint16_t> statuses = ledHandler.frontlightGetStatus();
  uint16_t arr[NUM_SCREENS];
  std::copy(statuses.begin(), statuses.end(), arr);

  JsonArray fl = root["flStatus"].to<JsonArray>();
  copyArray(arr, fl);

  if (hasLightLevel())
    root["lightLevel"] = getLightLevel();
#endif

  JsonObject dnd = root["dnd"].to<JsonObject>();
  dnd["enabled"] = ledHandler.isDNDEnabled();
  dnd["dndTimeEnabled"] = ledHandler.isDNDTimeBasedEnabled();
  dnd["startTime"] = String(ledHandler.getDNDStartHour()) + ":" +
                     (ledHandler.getDNDStartMinute() < 10 ? "0" : "") +
                     String(ledHandler.getDNDStartMinute());
  dnd["endTime"] = String(ledHandler.getDNDEndHour()) + ":" +
                   (ledHandler.getDNDEndMinute() < 10 ? "0" : "") +
                   String(ledHandler.getDNDEndMinute());
  dnd["active"] = ledHandler.isDNDActive();

  root["leds"] = buildLedStatusJson()["data"];

  std::array<String, NUM_SCREENS> epdContent =
      EPDManager::getInstance().getCurrentContent();
  JsonArray data = root["data"].to<JsonArray>();
  for (const auto &content : epdContent)
    data.add(content);

  return root;
}

void eventSourceUpdate() {
  if (!events.count())
    return;

  JsonDocument root = buildStatusJson();

  String buffer;
  serializeJson(root, buffer);
  events.send(buffer.c_str(), "status");
}

static void onApiStatus(AsyncWebServerRequest *request) {
  if (requireHttpAuth(request))
    return;
  AsyncResponseStream *response = request->beginResponseStream(JSON_CONTENT);
  JsonDocument root = buildStatusJson();
  serializeJson(root, *response);
  request->send(response);
}

static void onApiSystemStatus(AsyncWebServerRequest *request) {
  if (requireHttpAuth(request))
    return;
  AsyncResponseStream *response = request->beginResponseStream(JSON_CONTENT);

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

void registerStatusRoutes() {
  server.on("/api/status", AsyncWebRequestMethod::HTTP_GET, onApiStatus);
  server.on("/api/system_status", AsyncWebRequestMethod::HTTP_GET, onApiSystemStatus);
}
