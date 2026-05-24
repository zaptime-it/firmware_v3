#pragma once

#include <Arduino.h>
#include <MCP23017.h>
#include <Preferences.h>
#include <WiFiClientSecure.h>
#include <WiFiManager.h>
#include <base64.h>
#include <esp_task_wdt.h>
#include <map>
#include <nvs_flash.h>

#include "lib/data_sources/block_notify.hpp"
#include "lib/drivers/buttons/button_handler.hpp"
#include "lib/drivers/epd/epd.hpp"
// #include "lib/improv.hpp"
#include "lib/data_sources/bitaxe_fetch.hpp"
#include "lib/data_sources/mining_pool_stats_fetch.hpp"
#include "lib/data_sources/nostr_notify.hpp"
#include "lib/drivers/leds/led_handler.hpp"
#include "lib/net/ota/ota.hpp"

#include "lib/data_sources/v2_notify.hpp"

#include "lib/data_sources/price_notify.hpp"
#include "lib/net/webserver/webserver.hpp"
#include "lib/system/shared.hpp"
#include "lib/ui/screen_handler.hpp"
#ifdef HAS_FRONTLIGHT
#include "BH1750.h"
#include "PCA9685.h"
#endif

#include "defaults.hpp"
#include "timezone_data.hpp"
#define NTP_SERVER "pool.ntp.org"
#ifndef MCP_DEV_ADDR
#define MCP_DEV_ADDR 0x20
#endif

void setup();
void syncTime();
void setTimezone(String timezone);
uint getLastTimeSync();
void setupPreferences();
void setupWebsocketClients(void *pvParameters);
void setupHardware();
void setupWifi();
void setupTimers();
void finishSetup();
void setupMcp();
#ifdef HAS_FRONTLIGHT
extern BH1750 bh1750;
extern bool hasLuxSensor;
float getLightLevel();
bool hasLightLevel();
#endif

String getMyHostname();
std::vector<ScreenMapping> getScreenNameMap();

std::vector<std::string> getLocalUrl();
// bool improv_connectWifi(std::string ssid, std::string password);
// void improvGetAvailableWifiNetworks();
// bool onImprovCommandCallback(improv::ImprovCommand cmd);
// void onImprovErrorCallback(improv::Error err);
// void improv_set_state(improv::State state);
// void improv_send_response(std::vector<uint8_t> &response);
// void improv_set_error(improv::Error error);
// void addCurrencyMappings(const std::vector<std::string>& currencies);
std::vector<std::string> getActiveCurrencies();
std::vector<std::string> getAvailableCurrencies();

bool isActiveCurrency(std::string &currency);

void WiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info);
String getHwRev();
bool isWhiteVersion();
String getFsRev();

bool debugLogEnabled();

void addScreenMapping(int value, const char *name);
// void addScreenMapping(int value, const String& name);
// void addScreenMapping(int value, const std::string& name);

// Rebuild screenMappings from the PrefKeys::ScreenOrder NVS value merged
// against the current feature-gated catalog. Safe to call at runtime after
// a PATCH to /api/settings changes the order; guarded by an internal mutex.
void rebuildScreenMappings();

int findScreenIndexByValue(int value);
String replaceAmbiguousChars(String input);
const char *getFirmwareFilename();
const char *getWebUiFilename();
// void loadIcons();

extern Preferences preferences;
extern MCP23017 mcp1;
#ifdef IS_BTCLOCK_V8
extern MCP23017 mcp2;
#endif

#ifdef HAS_FRONTLIGHT
extern PCA9685 flArray;
#endif

// Expose DataSourceType enum
extern DataSourceType getDataSource();
extern void setDataSource(DataSourceType source);