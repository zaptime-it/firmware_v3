#pragma once

// Public surface of the split webserver module. Everything below is what
// the rest of the firmware (src/main.cpp, lib/system/config.cpp,
// lib/drivers/epd, lib/drivers/leds, lib/system/timers, lib/ui, lib/net/ota)
// reaches into; the internal wiring lives in net/webserver/internal.hpp and
// the per-concern cpp files.
//
// NOTE: include order here matters. WebServer.h must come before
// ESPAsyncWebServer.h, otherwise the latter pulls in the wrong symbols and
// the build breaks with "redefinition of 'class WebServer'".
#include "WebServer.h"
#include "ESPAsyncWebServer.h"
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <ESPmDNS.h>
#include "AsyncJson.h"
#include <iostream>

#include "lib/data_sources/block_notify.hpp"
#include "lib/data_sources/price_notify.hpp"
#include "lib/ui/screen_handler.hpp"
#include "OneParamRewrite.hpp"
#include "lib/data_sources/mining_pool/pool_factory.hpp"

// FreeRTOS task that fans the /api/status JSON out over SSE whenever any
// state-changing component calls xTaskNotifyGive(eventSourceTaskHandle).
// Kept global so the EPD/LED/timer/OTA code can notify without having to
// go through a heavier callback layer.
extern TaskHandle_t eventSourceTaskHandle;

void setupWebserver();
void stopWebServer();
