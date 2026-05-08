#include "internal.hpp"

#include "lib/drivers/leds/led_handler.hpp"
#include "lib/system/pref_keys.hpp"
#include "lib/system/shared.hpp"
#include "lib/system/timers.hpp"
#include "screen_order.hpp"

#include <algorithm>
#include <set>
#include <vector>

// The three arrays below drive the generic branch of onApiSettingsPatch and
// the schema exposed by onApiSettingsGet. Routing keys through PrefKeys
// guarantees the identifier matches the backing NVS slot and makes it easy
// to see every PATCH-accepted setting in one place.
static const char *const PROGMEM strSettings[] = {
    PrefKeys::HostnamePrefix, PrefKeys::MempoolInstance,
    PrefKeys::NostrPubKey,    PrefKeys::NostrRelay,
    PrefKeys::BitaxeHostname, PrefKeys::MiningPoolName,
    PrefKeys::MiningPoolUser, PrefKeys::NostrZapPubkey,
    PrefKeys::HttpAuthUser,   PrefKeys::HttpAuthPass,
    PrefKeys::OtaPass,        PrefKeys::GitReleaseUrl,
    PrefKeys::PoolLogosUrl,   PrefKeys::CeEndpoint,
    PrefKeys::FontName,       PrefKeys::LocalPoolHost,
    PrefKeys::TzString};

static const char *const PROGMEM uintSettings[] = {
    PrefKeys::MinSecPriceUpd, PrefKeys::FullRefreshMin,
    PrefKeys::LedBrightness,  PrefKeys::FlMaxBrightness,
    PrefKeys::FlEffectDelay,  PrefKeys::LuxLightToggle,
    PrefKeys::WpTimeout,      PrefKeys::BlockFlashColor};

static const char *const PROGMEM boolSettings[] = {
    PrefKeys::LedTestOnPower,  PrefKeys::LedFlashOnUpd,
    PrefKeys::MdnsEnabled,     PrefKeys::OtaEnabled,
    PrefKeys::StealFocus,      PrefKeys::McapBigChar,
    PrefKeys::UseSatsSymbol,   PrefKeys::UseBlkCountdown,
    PrefKeys::SuffixPrice,     PrefKeys::DisableLeds,
    PrefKeys::MowMode,         PrefKeys::SuffixShareDot,
    PrefKeys::FlOffWhenDark,   PrefKeys::FlAlwaysOn,
    PrefKeys::FlDisable,       PrefKeys::FlFlashOnUpd,
    PrefKeys::MempoolSecure,   PrefKeys::BitaxeEnabled,
    PrefKeys::MiningPoolStats, PrefKeys::VerticalDesc,
    PrefKeys::NostrZapNotify,  PrefKeys::HttpAuthEnabled,
    PrefKeys::EnableDebugLog,  PrefKeys::CeDisableSSL,
    PrefKeys::DndEnabled,      PrefKeys::DndTimeEnabled,
    PrefKeys::ScrnRestoreZap,  PrefKeys::BlockFeeDec,
    PrefKeys::SupplyPercent,   PrefKeys::RefrScrnChange,
    PrefKeys::InverseButtons,  PrefKeys::UseMscwTime,
    PrefKeys::PoolGlobalStats};

static void onApiSettingsGet(AsyncWebServerRequest *request) {
  if (requireHttpAuth(request))
    return;

  JsonDocument root;
  root["numScreens"] = NUM_SCREENS;
  root["invertedColor"] = preferences.getBool(
      "invertedColor",
      EPDManager::getInstance().getForegroundColor() == GxEPD_WHITE);
  root["timerSeconds"] = getTimerSeconds();
  root["timerRunning"] = isTimerActive();
  root["minSecPriceUpd"] = preferences.getUInt(
      "minSecPriceUpd", DEFAULT_SECONDS_BETWEEN_PRICE_UPDATE);
  root["fullRefreshMin"] =
      preferences.getUInt("fullRefreshMin", DEFAULT_MINUTES_FULL_REFRESH);
  root["wpTimeout"] = preferences.getUInt("wpTimeout", DEFAULT_WP_TIMEOUT);
  root["tzString"] = preferences.getString("tzString", DEFAULT_TZ_STRING);

  root["dataSource"] = preferences.getUChar("dataSource", DEFAULT_DATA_SOURCE);
  root["mempoolInstance"] =
      preferences.getString("mempoolInstance", DEFAULT_MEMPOOL_INSTANCE);
  root["mempoolSecure"] =
      preferences.getBool("mempoolSecure", DEFAULT_MEMPOOL_SECURE);

  root["localPoolHost"] =
      preferences.getString("localPoolHost", DEFAULT_LOCAL_POOL_ENDPOINT);

  root["nostrPubKey"] =
      preferences.getString("nostrPubKey", DEFAULT_NOSTR_NPUB);
  root["nostrRelay"] = preferences.getString("nostrRelay", DEFAULT_NOSTR_RELAY);
  root["nostrZapNotify"] =
      preferences.getBool("nostrZapNotify", DEFAULT_ZAP_NOTIFY_ENABLED);
  root["nostrZapPubkey"] =
      preferences.getString("nostrZapPubkey", DEFAULT_ZAP_NOTIFY_PUBKEY);
  root["ledFlashOnZap"] =
      preferences.getBool("ledFlashOnZap", DEFAULT_LED_FLASH_ON_ZAP);
  root["scrnRestoreZap"] =
      preferences.getBool("scrnRestoreZap", DEFAULT_SCREEN_RESTORE_AFTER_ZAP);
  root["fontName"] = preferences.getString("fontName", DEFAULT_FONT_NAME);
  root["availableFonts"] = FontNames::getAvailableFonts();

  root["ledTestOnPower"] =
      preferences.getBool("ledTestOnPower", DEFAULT_LED_TEST_ON_POWER);
  root["ledFlashOnUpd"] =
      preferences.getBool("ledFlashOnUpd", DEFAULT_LED_FLASH_ON_UPD);
  root["ledBrightness"] =
      preferences.getUInt("ledBrightness", DEFAULT_LED_BRIGHTNESS);
  root["stealFocus"] = preferences.getBool("stealFocus", DEFAULT_STEAL_FOCUS);
  root["mcapBigChar"] =
      preferences.getBool("mcapBigChar", DEFAULT_MCAP_BIG_CHAR);
  root["mdnsEnabled"] =
      preferences.getBool("mdnsEnabled", DEFAULT_MDNS_ENABLED);
  root["otaEnabled"] = preferences.getBool("otaEnabled", DEFAULT_OTA_ENABLED);
  root["useSatsSymbol"] =
      preferences.getBool("useSatsSymbol", DEFAULT_USE_SATS_SYMBOL);
  root["useMscwTime"] =
      preferences.getBool("useMscwTime", DEFAULT_USE_MSCW_TIME);
  root["useBlkCountdown"] =
      preferences.getBool("useBlkCountdown", DEFAULT_USE_BLOCK_COUNTDOWN);
  root["suffixPrice"] =
      preferences.getBool("suffixPrice", DEFAULT_SUFFIX_PRICE);
  root["disableLeds"] =
      preferences.getBool("disableLeds", DEFAULT_DISABLE_LEDS);
  root["mowMode"] = preferences.getBool("mowMode", DEFAULT_MOW_MODE);
  root["verticalDesc"] =
      preferences.getBool("verticalDesc", DEFAULT_VERTICAL_DESC);
  root["blockFeeDec"] =
      preferences.getBool("blockFeeDec", DEFAULT_BLOCK_FEE_DECIMALS);
  root["blockFlashColor"] =
      preferences.getUInt("blockFlashColor", DEFAULT_BLOCK_FLASH_COLOR);
  root["supplyPercent"] =
      preferences.getBool("supplyPercent", DEFAULT_SUPPLY_PERCENT);
  root["refrScrnChange"] =
      preferences.getBool("refrScrnChange", DEFAULT_REFRESH_ON_SCREEN_CHANGE);
  root["inverseButtons"] =
      preferences.getBool("inverseButtons", DEFAULT_INVERSE_BUTTONS);
  root["suffixShareDot"] =
      preferences.getBool("suffixShareDot", DEFAULT_SUFFIX_SHARE_DOT);
  root["enableDebugLog"] =
      preferences.getBool("enableDebugLog", DEFAULT_ENABLE_DEBUG_LOG);

  root["hostnamePrefix"] =
      preferences.getString("hostnamePrefix", DEFAULT_HOSTNAME_PREFIX);
  root["hostname"] = getMyHostname();
  root["ip"] = WiFi.localIP();
  root["txPower"] = WiFi.getTxPower();

  root["gitReleaseUrl"] =
      preferences.getString("gitReleaseUrl", DEFAULT_GIT_RELEASE_URL);

  root["bitaxeEnabled"] =
      preferences.getBool("bitaxeEnabled", DEFAULT_BITAXE_ENABLED);
  root["bitaxeHostname"] =
      preferences.getString("bitaxeHostname", DEFAULT_BITAXE_HOSTNAME);

  root["miningPoolStats"] =
      preferences.getBool("miningPoolStats", DEFAULT_MINING_POOL_STATS_ENABLED);
  root["miningPoolName"] =
      preferences.getString("miningPoolName", DEFAULT_MINING_POOL_NAME);
  root["miningPoolUser"] =
      preferences.getString("miningPoolUser", DEFAULT_MINING_POOL_USER);
  root["poolGlobalStats"] =
      preferences.getBool("poolGlobalStats", DEFAULT_POOL_GLOBAL_STATS);
  root["availablePools"] = PoolFactory::getAvailablePools();
  root["httpAuthEnabled"] =
      preferences.getBool("httpAuthEnabled", DEFAULT_HTTP_AUTH_ENABLED);
  root["httpAuthUser"] =
      preferences.getString("httpAuthUser", DEFAULT_HTTP_AUTH_USERNAME);
  // Never ship the raw password to the client. Expose a boolean flag instead
  // so the UI can show "password set" without giving out credentials.
  // Reflect whether the user has *explicitly* stored a password (non-empty
  // NVS value), not whether the runtime-effective value is non-empty. The
  // previous default was DEFAULT_HTTP_AUTH_PASSWORD, so httpAuthPassSet
  // always reported true even when the device was still on factory defaults.
  root["httpAuthPassSet"] =
      preferences.getString("httpAuthPass", "").length() > 0;
  // Do not expose the ArduinoOTA password either; just tell the UI whether
  // one is configured so it can show a "password set" indicator.
  root["otaPassSet"] = preferences.getString("otaPass", "").length() > 0;

#ifdef HAS_FRONTLIGHT
  root["hasFrontlight"] = true;
  root["flDisable"] = preferences.getBool("flDisable");
  root["flMaxBrightness"] =
      preferences.getUInt("flMaxBrightness", DEFAULT_FL_MAX_BRIGHTNESS);
  root["flAlwaysOn"] = preferences.getBool("flAlwaysOn", DEFAULT_FL_ALWAYS_ON);
  root["flEffectDelay"] =
      preferences.getUInt("flEffectDelay", DEFAULT_FL_EFFECT_DELAY);
  root["flFlashOnUpd"] =
      preferences.getBool("flFlashOnUpd", DEFAULT_FL_FLASH_ON_UPDATE);
  root["flFlashOnZap"] =
      preferences.getBool("flFlashOnZap", DEFAULT_FL_FLASH_ON_ZAP);
  root["hasLightLevel"] = hasLightLevel();
  root["luxLightToggle"] =
      preferences.getUInt("luxLightToggle", DEFAULT_LUX_LIGHT_TOGGLE);
  root["flOffWhenDark"] =
      preferences.getBool("flOffWhenDark", DEFAULT_FL_OFF_WHEN_DARK);
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
  for (int i = 0; i < screenNameMap.size(); i++) {
    JsonObject o = screens.add<JsonObject>();
    String key = "screen" + String(screenNameMap.at(i).value) + "Visible";
    o["id"] = screenNameMap.at(i).value;
    o["name"] = String(screenNameMap.at(i).name);
    o["enabled"] = preferences.getBool(key.c_str(), true);
    // `order` reflects the current rotation position. Emitted explicitly
    // so clients don't need to trust JsonArray iteration order, and so a
    // reorder PATCH has an unambiguous field to write back into.
    o["order"] = i;
  }

  root["poolLogosUrl"] =
      preferences.getString("poolLogosUrl", DEFAULT_MINING_POOL_LOGOS_URL);
  root["ceEndpoint"] =
      preferences.getString("ceEndpoint", DEFAULT_CUSTOM_ENDPOINT);
  root["ceDisableSSL"] =
      preferences.getBool("ceDisableSSL", DEFAULT_CUSTOM_ENDPOINT_DISABLE_SSL);

  auto &ledHandler = getLedHandler();
  JsonObject dnd = root["dnd"].to<JsonObject>();
  dnd["enabled"] = ledHandler.isDNDEnabled();
  dnd["dndTimeEnabled"] = ledHandler.isDNDTimeBasedEnabled();
  dnd["startHour"] = ledHandler.getDNDStartHour();
  dnd["startMinute"] = ledHandler.getDNDStartMinute();
  dnd["endHour"] = ledHandler.getDNDEndHour();
  dnd["endMinute"] = ledHandler.getDNDEndMinute();

  AsyncResponseStream *response = request->beginResponseStream(JSON_CONTENT);
  serializeJson(root, *response);
  request->send(response);
}

static void onApiSettingsPatch(AsyncWebServerRequest *request,
                               JsonVariant &json) {
  if (requireHttpAuth(request))
    return;

  JsonObject settings = json.as<JsonObject>();
  bool settingsChanged = true;

  if (settings["invertedColor"].is<bool>()) {
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

  if (settings["timePerScreen"].is<uint>()) {
    preferences.putUInt("timerSeconds",
                        settings["timePerScreen"].as<uint>() * 60);
  }

  for (String setting : strSettings) {
    if (settings[setting].is<String>()) {
      preferences.putString(setting.c_str(), settings[setting].as<String>());
      Serial.printf("set %s=%s\r\n", setting.c_str(),
                    settings[setting].as<String>().c_str());
    }
  }

  for (String setting : uintSettings) {
    if (settings[setting].is<uint>()) {
      preferences.putUInt(setting.c_str(), settings[setting].as<uint>());
      Serial.printf("set %s=%u\r\n", setting.c_str(),
                    settings[setting].as<uint>());
    }
  }

  if (settings["tzOffset"].is<int>()) {
    int gmtOffset = settings["tzOffset"].as<int>() * 60;
    preferences.putInt("gmtOffset", gmtOffset);
  }

  for (String setting : boolSettings) {
    if (settings[setting].is<bool>()) {
      bool value = settings[setting].as<bool>();
      // DND bools need to go through LedHandler so the in-memory state used
      // by isDNDActive() stays in sync with NVS. The generic putBool path
      // below only updated NVS, which left the handler running effects
      // until the next reboot.
      if (setting == "dndEnabled") {
        getLedHandler().setDNDEnabled(value);
      } else if (setting == "dndTimeEnabled") {
        getLedHandler().setDNDTimeBasedEnabled(value);
      } else {
        preferences.putBool(setting.c_str(), value);
      }
      Serial.printf("set %s=%d\r\n", setting.c_str(), value);
    }
  }

  if (settings["screens"].is<JsonArray>()) {
    JsonArray incoming = settings["screens"].as<JsonArray>();

    // Detect whether this is a reorder PATCH or a visibility-only PATCH.
    // Reorder requires every entry to carry an `order`; a partial order
    // is ambiguous (what positions do the unsent screens get?) and is
    // rejected rather than silently applied.
    bool anyOrder = false;
    bool allOrder = true;
    for (JsonVariant screen : incoming) {
      JsonObject s = screen.as<JsonObject>();
      if (s["order"].is<int>() || s["order"].is<uint>())
        anyOrder = true;
      else
        allOrder = false;
    }
    if (anyOrder && !allOrder) {
      request->send(HTTP_BAD_REQUEST, JSON_CONTENT,
                    "{\"error\":\"partial screen order: every entry must "
                    "include 'order' or none\"}");
      return;
    }

    if (anyOrder) {
      // Validate against the catalog returned by getScreenNameMap(),
      // which is the same set the WebUI saw on the preceding GET. Any ID
      // outside that set (98/99 mode overrides, unknown, feature-gated
      // off) is a programming error on the client and we reject loudly.
      std::vector<ScreenMapping> catalog = getScreenNameMap();
      std::set<int> catalogIds;
      for (const auto &m : catalog)
        catalogIds.insert(m.value);

      const size_t n = incoming.size();
      std::vector<std::pair<int, int>> pairs; // (order, id)
      pairs.reserve(n);
      std::set<int> seenIds;
      std::set<int> seenOrders;

      for (JsonVariant screen : incoming) {
        JsonObject s = screen.as<JsonObject>();
        int id = s["id"].as<int>();
        int order = s["order"].as<int>();

        if (!catalogIds.count(id)) {
          request->send(HTTP_BAD_REQUEST, JSON_CONTENT,
                        "{\"error\":\"unknown screen id in order\"}");
          return;
        }
        if (!seenIds.insert(id).second) {
          request->send(HTTP_BAD_REQUEST, JSON_CONTENT,
                        "{\"error\":\"duplicate screen id in order\"}");
          return;
        }
        if (order < 0 || order >= (int)n) {
          request->send(
              HTTP_BAD_REQUEST, JSON_CONTENT,
              "{\"error\":\"screen order out of range; expected 0..n-1\"}");
          return;
        }
        if (!seenOrders.insert(order).second) {
          request->send(HTTP_BAD_REQUEST, JSON_CONTENT,
                        "{\"error\":\"duplicate order value\"}");
          return;
        }
        pairs.emplace_back(order, id);
      }
      if (seenIds.size() != catalogIds.size()) {
        // Not every rotatable screen was mentioned — a reorder must cover
        // the full set. Otherwise we can't produce a coherent linear order.
        request->send(
            HTTP_BAD_REQUEST, JSON_CONTENT,
            "{\"error\":\"screen order must include every rotatable screen\"}");
        return;
      }

      std::sort(pairs.begin(), pairs.end());
      std::vector<int> orderedIds;
      orderedIds.reserve(pairs.size());
      for (const auto &p : pairs)
        orderedIds.push_back(p.second);

      preferences.putString(PrefKeys::ScreenOrder,
                            btclock::serializeScreenOrder(orderedIds).c_str());
      rebuildScreenMappings();
      // Restart the periodic esp_timer so the user sees the new sequence
      // immediately rather than waiting out the remainder of the current
      // timerSeconds interval.
      resetScreenRotateTimer();
    }

    for (JsonVariant screen : incoming) {
      JsonObject s = screen.as<JsonObject>();
      uint id = s["id"].as<uint>();
      String prefKey = "screen" + String(id) + "Visible";
      bool visible = s["enabled"].as<bool>();
      preferences.putBool(prefKey.c_str(), visible);
    }
  }

  if (settings["actCurrencies"].is<JsonArray>()) {
    String actCurrencies;
    for (JsonVariant cur : settings["actCurrencies"].as<JsonArray>()) {
      if (!actCurrencies.isEmpty())
        actCurrencies += ",";
      actCurrencies += cur.as<String>();
    }
    preferences.putString("actCurrencies", actCurrencies.c_str());
  }

  if (settings["txPower"].is<int>()) {
    int txPower = settings["txPower"].as<int>();

    if (txPower == 80) {
      preferences.remove("txPower");
      if (WiFi.getTxPower() != 80)
        ESP.restart();
    } else if (static_cast<int>(wifi_power_t::WIFI_POWER_MINUS_1dBm) <=
                   txPower &&
               txPower <= static_cast<int>(wifi_power_t::WIFI_POWER_19_5dBm)) {
      if (WiFi.setTxPower(static_cast<wifi_power_t>(txPower))) {
        preferences.putInt("txPower", txPower);
        settingsChanged = true;
      }
    }
  }

  if (settings["dataSource"].is<uint8_t>()) {
    uint8_t dataSource = settings["dataSource"].as<uint8_t>();
    if (dataSource <= CUSTOM_SOURCE) {
      preferences.putUChar("dataSource", dataSource);
      settingsChanged = true;
    }
  }

  if (settings["ceEndpoint"].is<String>()) {
    preferences.putString("ceEndpoint", settings["ceEndpoint"].as<String>());
    settingsChanged = true;
  }

  if (settings["dnd"].is<JsonObject>()) {
    JsonObject dndObj = settings["dnd"];
    auto &ledHandler = getLedHandler();

    if (dndObj["dndTimeEnabled"].is<bool>()) {
      ledHandler.setDNDTimeBasedEnabled(dndObj["dndTimeEnabled"].as<bool>());
    }
    if (dndObj["startHour"].is<uint8_t>() &&
        dndObj["startMinute"].is<uint8_t>() &&
        dndObj["endHour"].is<uint8_t>() && dndObj["endMinute"].is<uint8_t>()) {
      ledHandler.setDNDTimeRange(dndObj["startHour"].as<uint8_t>(),
                                 dndObj["startMinute"].as<uint8_t>(),
                                 dndObj["endHour"].as<uint8_t>(),
                                 dndObj["endMinute"].as<uint8_t>());
    }
  }

  request->send(HTTP_OK);
  if (settingsChanged) {
    getLedHandler().queueEffect(LED_FLASH_SUCCESS);
  }
  notifyEventSourceStatus();
}

void registerSettingsRoutes() {
  server.on("/api/settings", HTTP_GET, onApiSettingsGet);

  // PATCH /api/settings replaces the old POST-ish /api/json/settings so the
  // settings endpoint pair (GET read / PATCH write) lives on a single URL.
  AsyncCallbackJsonWebHandler *settingsPatchHandler =
      new AsyncCallbackJsonWebHandler("/api/settings", onApiSettingsPatch);
  settingsPatchHandler->setMethod(HTTP_PATCH);
  server.addHandler(settingsPatchHandler);
}
