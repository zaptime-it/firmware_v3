#pragma once

#include "MCP23017.h"
// #include <zlib_turbo.h>
#include <ArduinoJson.h>
#include <GxEPD2.h>
#include <GxEPD2_BW.h>
#include <Preferences.h>
#include <NetworkClient.h>
#include <NetworkClientSecure.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <mbedtls/md.h>
#include <mutex>

#include "esp_crt_bundle.h"
#include "lib/system/tls_gate.hpp"
#include <HTTPClient.h>
#include <Update.h>

#include <array>
#include <memory>
#include <mutex>
#include <string>
#include <utils.hpp>

#include "defaults.hpp"

#define USER_AGENT "BTClock/3.0"

extern MCP23017 mcp1;
#ifdef IS_BTCLOCK_V8
extern MCP23017 mcp2;
#endif
extern Preferences preferences;
extern std::mutex mcpMutex;

#ifdef VERSION_EPD_2_13
#define EPD_CLASS GxEPD2_213_B74
#endif

#ifdef VERSION_EPD_2_9
#define EPD_CLASS GxEPD2_290_T94
#endif

const PROGMEM int SCREEN_BLOCK_HEIGHT = 0;

const PROGMEM int SCREEN_TIME = 3;
const PROGMEM int SCREEN_HALVING_COUNTDOWN = 4;
const PROGMEM int SCREEN_BLOCK_FEE_RATE = 6;

const PROGMEM int SCREEN_SATS_PER_CURRENCY = 10;

const PROGMEM int SCREEN_BTC_TICKER = 20;

const PROGMEM int SCREEN_MARKET_CAP = 30;

const PROGMEM int SCREEN_BITCOIN_SUPPLY = 40;

const PROGMEM int SCREEN_MINING_POOL_STATS_HASHRATE = 70;
const PROGMEM int SCREEN_MINING_POOL_STATS_EARNINGS = 71;

const PROGMEM int SCREEN_BITAXE_HASHRATE = 80;
const PROGMEM int SCREEN_BITAXE_BESTDIFF = 81;

const PROGMEM int SCREEN_COUNTDOWN = 98;
const PROGMEM int SCREEN_CUSTOM = 99;
const int SCREEN_COUNT = 8;
const PROGMEM int screens[SCREEN_COUNT] = {
    SCREEN_BLOCK_HEIGHT, SCREEN_SATS_PER_CURRENCY, SCREEN_BTC_TICKER,
    SCREEN_TIME,         SCREEN_HALVING_COUNTDOWN, SCREEN_BLOCK_FEE_RATE,
    SCREEN_MARKET_CAP,   SCREEN_BITCOIN_SUPPLY};
const int usPerSecond = 1000000;
const int usPerMinute = 60 * usPerSecond;
const int msPerSecond = 1000;

// extern const char *github_root_ca;
// extern const char *isrg_root_x1cert;

extern const uint8_t
    rootca_crt_bundle_start[] asm("_binary_x509_crt_bundle_start");
extern const uint8_t rootca_crt_bundle_end[] asm("_binary_x509_crt_bundle_end");
// extern const uint8_t ocean_logo_comp[] asm("_binary_ocean_gz_start");
// extern const uint8_t ocean_logo_comp_end[] asm("_binary_ocean_gz_end");

// uint8_t* getOceanIcon();

// const size_t ocean_logo_size = ocean_logo_comp_end - ocean_logo_comp;

const PROGMEM char UPDATE_FIRMWARE = U_FLASH;
const PROGMEM char UPDATE_WEBUI = U_SPIFFS;
const PROGMEM char UPDATE_ALL = 99;

struct ScreenMapping {
  int value;
  const char *name;
};

String calculateSHA256(uint8_t *data, size_t len);
String calculateSHA256(NetworkClient *stream, size_t contentLength);

namespace ArduinoJson {
template <typename T> struct Converter<std::vector<T>> {
  static void toJson(const std::vector<T> &src, JsonVariant dst) {
    JsonArray array = dst.to<JsonArray>();
    for (T item : src)
      array.add(item);
  }
};

template <size_t N> struct Converter<std::array<String, N>> {
  static void toJson(const std::array<String, N> &src, JsonVariant dst) {
    JsonArray array = dst.to<JsonArray>();
    for (const String &item : src) {
      array.add(item);
    }
  }
};
} // namespace ArduinoJson

class HttpHelper {
public:
  static HTTPClient *begin(const String &url);
  static void end(HTTPClient *http);

  // RAII wrapper over `HTTPClient*` that also serialises access to the
  // shared static WiFi(Secure)Client instances below.
  //
  // Two reasons the lock is load-bearing:
  //   1. Multiple FreeRTOS tasks (BlockNotify setup,
  //      MiningPoolStatsFetch, Bitaxe fetcher, OTA) call beginScoped()
  //      concurrently. They all share the same static secureClient,
  //      whose mbedtls SSL context is not reentrancy-safe: two
  //      overlapping HTTPS requests corrupt each other's state, and
  //      the second NetworkClientSecure::stop() then trips a heap-
  //      poisoning assert ("CORRUPT HEAP: Bad head").
  //   2. Every HTTPS request allocates a fresh mbedtls context
  //      (~16 KB IN buffer + scratch). Doing that in parallel with
  //      the WS data-source handshakes is the dominant cause of
  //      TLS connect failures on Rev B with every feature enabled.
  //      tls_gate::mutex() is also taken by the WS reconnect path,
  //      so sharing it here forces one TLS handshake at a time
  //      across the whole firmware.
  struct HttpClientDeleter {
    std::unique_lock<std::mutex> lock;
    void operator()(HTTPClient *http) const { HttpHelper::end(http); }
  };
  using ScopedHttp = std::unique_ptr<HTTPClient, HttpClientDeleter>;
  static ScopedHttp beginScoped(const String &url) {
    std::unique_lock<std::mutex> lk(tls_gate::mutex());
    HTTPClient *http = begin(url);
    return ScopedHttp(http, HttpClientDeleter{std::move(lk)});
  }

private:
  static NetworkClientSecure secureClient;
  static NetworkClient insecureClient;
};