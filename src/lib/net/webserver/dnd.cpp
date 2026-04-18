#include "internal.hpp"

#include "lib/drivers/leds/led_handler.hpp"

// NOTE: the time-based DND handlers
// (onApiDNDTimeBasedEnable/Disable/SetTimeRange) that used to live here in
// the monolithic webserver.cpp were never registered on any route, were not
// used by the WebUI, and had no other consumers. They were removed as part
// of the Phase 4 split. The scheduled-window fields are still settable via
// PATCH /api/settings ("dnd" object), which is the path the UI actually
// uses.

static void onApiDNDStatus(AsyncWebServerRequest *request)
{
  if (requireHttpAuth(request)) return;
  auto &ledHandler = getLedHandler();

  JsonDocument doc;
  doc["enabled"]        = ledHandler.isDNDEnabled();
  doc["dndTimeEnabled"] = ledHandler.isDNDTimeBasedEnabled();
  doc["startTime"] = String(ledHandler.getDNDStartHour()) + ":" +
                     (ledHandler.getDNDStartMinute() < 10 ? "0" : "") +
                     String(ledHandler.getDNDStartMinute());
  doc["endTime"] = String(ledHandler.getDNDEndHour()) + ":" +
                   (ledHandler.getDNDEndMinute() < 10 ? "0" : "") +
                   String(ledHandler.getDNDEndMinute());
  doc["active"] = ledHandler.isDNDActive();

  String response;
  serializeJson(doc, response);
  request->send(HTTP_OK, "application/json", response);
}

static void onApiDNDEnable(AsyncWebServerRequest *request)
{
  if (requireHttpAuth(request)) return;
  getLedHandler().setDNDEnabled(true);
  request->send(HTTP_OK);
  notifyEventSourceStatus();
}

static void onApiDNDDisable(AsyncWebServerRequest *request)
{
  if (requireHttpAuth(request)) return;
  getLedHandler().setDNDEnabled(false);
  request->send(HTTP_OK);
  notifyEventSourceStatus();
}

void registerDndRoutes()
{
  server.on("/api/dnd/status",  HTTP_GET,  onApiDNDStatus);
  server.on("/api/dnd/enable",  HTTP_POST, onApiDNDEnable);
  server.on("/api/dnd/disable", HTTP_POST, onApiDNDDisable);
}
