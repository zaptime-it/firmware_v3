#include "internal.hpp"

#include "lib/drivers/leds/led_handler.hpp"
#include "lib/system/shared.hpp"

static void onApiLightsStatus(AsyncWebServerRequest *request)
{
  if (requireHttpAuth(request)) return;
  AsyncResponseStream *response = request->beginResponseStream(JSON_CONTENT);
  serializeJson(buildLedStatusJson()["data"], *response);
  request->send(response);
}

// Exposed in internal.hpp because onApiLightsSetJson delegates here when the
// incoming JSON body is empty.
void onApiLightsOff(AsyncWebServerRequest *request)
{
  if (requireHttpAuth(request)) return;
  getLedHandler().setLights(0, 0, 0);
  request->send(HTTP_OK);
  notifyEventSourceStatus();
}

static void onApiLightsSetColor(AsyncWebServerRequest *request)
{
  if (requireHttpAuth(request)) return;
  if (!request->hasParam("c"))
  {
    request->send(HTTP_BAD_REQUEST);
    return;
  }

  AsyncResponseStream *response = request->beginResponseStream(JSON_CONTENT);
  String rgbColor = request->getParam("c")->value();

  if (rgbColor.compareTo("off") == 0)
  {
    getLedHandler().setLights(0, 0, 0);
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
    getLedHandler().setLights(r, g, b);
  }

  serializeJson(buildLedStatusJson()["data"], *response);
  request->send(response);
  notifyEventSourceStatus();
}

static void onApiLightsSetJson(AsyncWebServerRequest *request, JsonVariant &json)
{
  if (requireHttpAuth(request)) return;
  auto &ledHandler = getLedHandler();
  auto &pixels     = ledHandler.getPixels();

  JsonArray lights = json.as<JsonArray>();

  if (lights.size() != pixels.numPixels())
  {
    if (!lights.size())
    {
      // empty array == "turn everything off" shortcut
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
      red   = lights[i]["red"].as<uint>();
      green = lights[i]["green"].as<uint>();
      blue  = lights[i]["blue"].as<uint>();
    }
    else if (lights[i]["hex"].is<const char *>())
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

#ifdef HAS_FRONTLIGHT
static void onApiFrontlightOn(AsyncWebServerRequest *request)
{
  if (requireHttpAuth(request)) return;
  getLedHandler().frontlightFadeInAll();
  request->send(HTTP_OK);
  notifyEventSourceStatus();
}

static void onApiFrontlightStatus(AsyncWebServerRequest *request)
{
  if (requireHttpAuth(request)) return;
  auto &ledHandler = getLedHandler();
  AsyncResponseStream *response = request->beginResponseStream(JSON_CONTENT);

  JsonDocument root;
  std::vector<uint16_t> statuses = ledHandler.frontlightGetStatus();
  uint16_t arr[NUM_SCREENS];
  std::copy(statuses.begin(), statuses.end(), arr);

  JsonArray data = root["flStatus"].to<JsonArray>();
  copyArray(arr, data);
  serializeJson(root, *response);
  request->send(response);
}

static void onApiFrontlightFlash(AsyncWebServerRequest *request)
{
  if (requireHttpAuth(request)) return;
  getLedHandler().frontlightFlash(preferences.getUInt("flEffectDelay"));
  request->send(HTTP_OK);
  notifyEventSourceStatus();
}

static void onApiFrontlightSetBrightness(AsyncWebServerRequest *request)
{
  if (requireHttpAuth(request)) return;
  if (!request->hasParam("b"))
  {
    request->send(HTTP_BAD_REQUEST);
    return;
  }
  getLedHandler().frontlightSetBrightness(request->getParam("b")->value().toInt());
  request->send(HTTP_OK);
  notifyEventSourceStatus();
}

static void onApiFrontlightOff(AsyncWebServerRequest *request)
{
  if (requireHttpAuth(request)) return;
  getLedHandler().frontlightFadeOutAll();
  request->send(HTTP_OK);
  notifyEventSourceStatus();
}
#endif

void registerLightsRoutes()
{
  AsyncCallbackJsonWebHandler *lightsJsonHandler =
      new AsyncCallbackJsonWebHandler("/api/lights/set", onApiLightsSetJson);
  lightsJsonHandler->setMethod(HTTP_POST);
  server.addHandler(lightsJsonHandler);

  server.on("/api/lights/off",   HTTP_POST, onApiLightsOff);
  server.on("/api/lights/color", HTTP_POST, onApiLightsSetColor);
  server.on("/api/lights",       HTTP_GET,  onApiLightsStatus);

  server.addRewrite(new OneParamRewrite("/api/lights/color/{color}",
                                        "/api/lights/color?c={color}"));

#ifdef HAS_FRONTLIGHT
  server.on("/api/frontlight/on",         HTTP_POST, onApiFrontlightOn);
  server.on("/api/frontlight/flash",      HTTP_POST, onApiFrontlightFlash);
  server.on("/api/frontlight/status",     HTTP_GET,  onApiFrontlightStatus);
  server.on("/api/frontlight/brightness", HTTP_POST, onApiFrontlightSetBrightness);
  server.on("/api/frontlight/off",        HTTP_POST, onApiFrontlightOff);

  server.addRewrite(
      new OneParamRewrite("/api/frontlight/brightness/{b}",
                          "/api/frontlight/brightness?b={b}"));
#endif
}
