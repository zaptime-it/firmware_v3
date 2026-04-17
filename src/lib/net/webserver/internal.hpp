#pragma once

// Internal header for the src/lib/net/webserver/ split. Everything declared
// here is module-private: the public surface of the webserver lives in
// webserver.hpp. Each concern-specific cpp file (status, settings, lights,
// dnd, actions, ota_routes) owns its handlers and exposes a single
// register<Group>Routes() entry point called from setupWebserver().
//
// Why the split exists at all: the old src/lib/webserver.cpp was a
// ~1.5 kLoC file that mixed wiring, status formatting, settings marshaling,
// LED/frontlight control, DND, actions, and OTA upload logic. Adding or
// reviewing any one of those concerns meant scrolling past all the others.

#include "webserver.hpp"

// HTTP status literals. Previously open-coded as raw integers in every
// handler. Put them here so the status discipline is obvious and any new
// handler trivially gets the standard set.
#define HTTP_OK                  200
#define HTTP_BAD_REQUEST         400
#define HTTP_NOT_FOUND           404
#define HTTP_SERVICE_UNAVAILABLE 503

extern const char *const JSON_CONTENT;

extern AsyncWebServer server;
extern AsyncEventSource events;

// requireHttpAuth: gate that sensitive endpoints call first. When HTTP auth
// is disabled in NVS it returns false immediately; otherwise it verifies
// Basic Auth credentials and, on failure, has already sent the 401 response
// so the caller just needs to return.
bool requireHttpAuth(AsyncWebServerRequest *request);

// Kick the SSE fan-out task. Nearly every state-changing handler calls this
// after it succeeds so the WebUI sees the new status without polling.
void notifyEventSourceStatus();

// Reboot the device cleanly after the current response has been flushed.
// Running esp_restart() directly from an AsyncTCP callback used to deadlock
// esp_wifi_stop() because the original path masked interrupts; we schedule
// a short-lived FreeRTOS task instead.
void scheduleDelayedRestart(uint32_t delayMs = 500);

// Shared status builders used by /api/status and SSE eventSourceUpdate().
// Both endpoints previously assembled an identical JSON tree independently,
// which was a steady source of drift when new fields were added.
JsonDocument buildStatusJson();
JsonDocument buildLedStatusJson();

// SSE push used by eventSourceTask() after a notify.
void eventSourceUpdate();

// Route-registration entry points, one per concern. setupWebserver()
// becomes a flat list of these calls so the wiring is the split's table
// of contents.
void registerStatusRoutes();
void registerSettingsRoutes();
void registerActionRoutes();
void registerLightsRoutes();
void registerDndRoutes();
void registerOtaRoutes();

// Used from actions.cpp (timer_restart, full_refresh) and ota_routes.cpp.
// Declared here so file-updates can reuse the same restart path.
void onApiRestart(AsyncWebServerRequest *request);

// Used from lights.cpp: the json-array handler delegates to the GET-style
// off handler when the incoming body is empty.
void onApiLightsOff(AsyncWebServerRequest *request);
