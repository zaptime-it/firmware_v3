#include "internal.hpp"

#include "lib/system/shared.hpp"

// Module-wide globals. Route files reach these through internal.hpp.
AsyncWebServer server(80);
AsyncEventSource events("/events");
TaskHandle_t eventSourceTaskHandle;

const char *const JSON_CONTENT = "application/json";

// Reboot from a dedicated task. Calling esp_restart() directly from an
// AsyncTCP callback (e.g. request->onDisconnect) used to be paired with
// noInterrupts(), which masked the scheduler tick; esp_wifi_stop() inside
// esp_restart() then blocked on a semaphore until the interrupt WDT fired
// and the device panicked. Running the delay + restart on a separate task
// keeps interrupts enabled and lets the AsyncTCP task finish cleanly before
// reboot.
void scheduleDelayedRestart(uint32_t delayMs) {
  static volatile bool s_restartScheduled = false;
  if (s_restartScheduled)
    return;
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
bool requireHttpAuth(AsyncWebServerRequest *request) {
  if (!preferences.getBool("httpAuthEnabled", DEFAULT_HTTP_AUTH_ENABLED)) {
    return false;
  }
  if (!request->authenticate(
          preferences.getString("httpAuthUser", DEFAULT_HTTP_AUTH_USERNAME)
              .c_str(),
          preferences.getString("httpAuthPass", DEFAULT_HTTP_AUTH_PASSWORD)
              .c_str())) {
    request->requestAuthentication();
    return true;
  }
  return false;
}

void notifyEventSourceStatus() {
  if (eventSourceTaskHandle != NULL)
    xTaskNotifyGive(eventSourceTaskHandle);
}

static void onNotFound(AsyncWebServerRequest *request) {
  // CORS preflight: 200 with the default CORS headers is enough for the
  // browser to then make the real request.
  if (request->method() == HTTP_OPTIONS) {
    request->send(HTTP_OK);
    return;
  }

  // Anything else really is 404. The old Sec-Fetch-Mode heuristic returned
  // 200 for fetch/XHR requests, which masked unknown endpoints and made the
  // API look like it was succeeding.
  request->send(HTTP_NOT_FOUND);
}

static void eventSourceTask(void *pvParameters) {
  for (;;) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    eventSourceUpdate();
  }
}

void setupWebserver() {
  events.onConnect([](AsyncEventSourceClient *client) {
    client->send("welcome", NULL, millis(), 1000);
  });
  server.addHandler(&events);

  AsyncStaticWebHandler &staticHandler =
      server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

  server.rewrite("/convert", "/");
  server.rewrite("/api", "/");

  if (preferences.getBool("httpAuthEnabled", DEFAULT_HTTP_AUTH_ENABLED)) {
    String authUser =
        preferences.getString("httpAuthUser", DEFAULT_HTTP_AUTH_USERNAME);
    String authPass =
        preferences.getString("httpAuthPass", DEFAULT_HTTP_AUTH_PASSWORD);
    staticHandler.setAuthentication(authUser, authPass);
    // EventSource / SSE is just a long-lived GET; when HTTP auth is on,
    // the stream leaks live device status to unauthenticated clients
    // unless we lock it too.
    events.setAuthentication(authUser.c_str(), authPass.c_str());
  }

  // Route table: one call per concern, each implemented in its sibling
  // file under src/lib/net/webserver/. Grep any of these names to jump to
  // the handlers for that group.
  registerStatusRoutes();
  registerSettingsRoutes();
  registerActionRoutes();
  registerLightsRoutes();
  registerDndRoutes();
  registerOtaRoutes();

  server.onNotFound(onNotFound);

  // CORS:
  // - For normal operation, the UI is served from the device itself.
  // - During WebUI development, the UI is often served from a local dev
  //   server (e.g. http://localhost:*), which must be able to call the
  //   device API.
  //
  // Because DefaultHeaders are global (not per-request), we can't
  // dynamically echo back the request Origin here. Use a permissive "*"
  // but keep headers restricted and rely on HTTP auth for state-changing
  // endpoints.
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods",
                                       "GET, PATCH, POST, OPTIONS");
  // Allow only the headers we actually need. "*" lets any client inject
  // arbitrary headers including Authorization, which combined with a
  // permissive Allow-Origin used to enable trivial CSRF.
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers",
                                       "Content-Type, Authorization");

  server.begin();

  if (preferences.getBool("mdnsEnabled", DEFAULT_MDNS_ENABLED)) {
    // Must not hang the whole device if mDNS fails to start (see prior
    // bug); one attempt, log on failure, continue.
    if (MDNS.begin(getMyHostname())) {
      MDNS.addService("http", "tcp", 80);
      MDNS.addServiceTxt("http", "tcp", "model", "BTClock");
      MDNS.addServiceTxt("http", "tcp", "version", "3.0");
      MDNS.addServiceTxt("http", "tcp", "rev", GIT_REV);
      MDNS.addServiceTxt("http", "tcp", "hw_rev", getHwRev());
    }
  }

  xTaskCreate(eventSourceTask, "eventSourceTask", 4096, NULL, tskIDLE_PRIORITY,
              &eventSourceTaskHandle);
}

void stopWebServer() { server.end(); }
