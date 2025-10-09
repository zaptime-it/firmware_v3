#include "config.hpp"
#include "led_handler.hpp"

#define MAX_ATTEMPTS_WIFI_CONNECTION 20

// zlib_turbo zt;

Preferences preferences;
MCP23017 mcp1(0x20);
#ifdef IS_BTCLOCK_V8
MCP23017 mcp2(0x21);
#endif

#ifdef HAS_FRONTLIGHT
PCA9685 flArray(PCA_I2C_ADDR);
BH1750 bh1750;
bool hasLuxSensor = false;
#endif

std::vector<ScreenMapping> screenMappings;
std::mutex mcpMutex;
uint lastTimeSync;

void addScreenMapping(int value, const char *name)
{
  screenMappings.push_back({value, name});
}

void setupDataSource()
{
  DataSourceType dataSource = getDataSource();
  bool zapNotifyEnabled = preferences.getBool("nostrZapNotify", DEFAULT_ZAP_NOTIFY_ENABLED);
  
  // Setup Nostr if it's either the data source or zap notifications are enabled
  if (dataSource == NOSTR_SOURCE || zapNotifyEnabled) {
    setupNostrNotify(dataSource == NOSTR_SOURCE, zapNotifyEnabled);
    setupNostrTask();
  }
  // Setup other data sources if Nostr is not the data source
  if (dataSource != NOSTR_SOURCE) {
    xTaskCreate(setupWebsocketClients, "setupWebsocketClients", 8192, NULL,
              tskIDLE_PRIORITY, NULL);
  }
}

void setup()
{
  setupPreferences();
  setupHardware();

  EPDManager::getInstance().initialize();
  if (preferences.getBool("ledTestOnPower", DEFAULT_LED_TEST_ON_POWER))
  {
    auto& ledHandler = getLedHandler();
    ledHandler.queueEffect(LED_POWER_TEST);
  }
  {
    std::lock_guard<std::mutex> lockMcp(mcpMutex);
    if (mcp1.read1(3) == LOW)
    {
      preferences.putBool("wifiConfigured", false);
      preferences.remove("txPower");

      WiFi.eraseAP();
      auto& ledHandler = getLedHandler();
      ledHandler.queueEffect(LED_EFFECT_WIFI_ERASE_SETTINGS);
    }
  }

  {
    if (mcp1.read1(0) == LOW)
    {
      // Then loop forever to prevent anything else from writing to the screen
      while (true)
      {
        delay(1000);
      }
    }
    else if (mcp1.read1(1) == LOW)
    {
      preferences.clear();
      auto& ledHandler = getLedHandler();
      ledHandler.queueEffect(LED_EFFECT_WIFI_ERASE_SETTINGS);
      nvs_flash_erase();
      delay(1000);

      ESP.restart();
    }
  }

  setupWifi();
  //  loadIcons();

  setupWebserver();

  syncTime();
  finishSetup();

  setupTasks();
  setupTimers();

  // Setup data sources (includes Nostr zap notifications if enabled)
  setupDataSource();

  if (preferences.getBool("bitaxeEnabled", DEFAULT_BITAXE_ENABLED))
  {
    BitaxeFetch::getInstance().setup();
  }

  if (preferences.getBool("miningPoolStats", DEFAULT_MINING_POOL_STATS_ENABLED))
  {
    MiningPoolStatsFetch::getInstance().setup();
  }

  ButtonHandler::setup();
  setupOTA();

  EPDManager::getInstance().waitUntilNoneBusy();

#ifdef HAS_FRONTLIGHT
  if (!preferences.getBool("flAlwaysOn", DEFAULT_FL_ALWAYS_ON))
  {
    auto& ledHandler = getLedHandler();
    ledHandler.frontlightFadeOutAll(preferences.getUInt("flEffectDelay"), true);
    flArray.allOFF();
  }
#endif

  EPDManager::getInstance().forceFullRefresh();
}

void setupWifi()
{
  WiFi.onEvent(WiFiEvent);
  
  // wifi_country_t country = {
  //   .cc = "NL",
  //   .schan = 1,
  //   .nchan = 13,
  //   .policy = WIFI_COUNTRY_POLICY_MANUAL
  // };

  // esp_err_t err = esp_wifi_set_country(&country);
  // if (err != ESP_OK) {
  //   Serial.printf("Failed to set country: %d\n", err);
  // }

  WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);
  WiFi.begin();



  if (preferences.getInt("txPower", DEFAULT_TX_POWER))
  {
    if (WiFi.setTxPower(
            static_cast<wifi_power_t>(preferences.getInt("txPower", DEFAULT_TX_POWER))))
    {
      Serial.printf("WiFi max tx power set to %d\n",
                    preferences.getInt("txPower", DEFAULT_TX_POWER));
    }
  }

  // if (!preferences.getBool("wifiConfigured", DEFAULT_WIFI_CONFIGURED)
  {

    auto& ledHandler = getLedHandler();
    ledHandler.queueEffect(LED_EFFECT_WIFI_WAIT_FOR_CONFIG);

    bool buttonPress = false;
    {
      std::lock_guard<std::mutex> lockMcp(mcpMutex);
      buttonPress = (mcp1.read1(2) == LOW);
    }

    {
      WiFiManager wm;

      byte mac[6];
      WiFi.macAddress(mac);
      String softAP_SSID = getMyHostname();
      WiFi.setHostname(softAP_SSID.c_str());
      String softAP_password = replaceAmbiguousChars(
          base64::encode(String(mac[2], 16) + String(mac[4], 16) +
                         String(mac[5], 16) + String(mac[1], 16) + String(mac[3], 16))
              .substring(2, 10));

      wm.setConfigPortalTimeout(preferences.getUInt("wpTimeout", DEFAULT_WP_TIMEOUT));
      wm.setWiFiAutoReconnect(false);
      wm.setDebugOutput(false);
      wm.setCountry("NL");
      wm.setConfigPortalBlocking(true);

      wm.setAPCallback([&](WiFiManager *wifiManager)
                       {
        Serial.printf("Entered config mode:ip=%s, ssid='%s', pass='%s'\n",
        WiFi.softAPIP().toString().c_str(),
        wifiManager->getConfigPortalSSID().c_str(),
        softAP_password.c_str());
        // delay(6000);
        EPDManager::getInstance().setForegroundColor(GxEPD_BLACK);
        EPDManager::getInstance().setBackgroundColor(GxEPD_WHITE);
        const String qrText = "qrWIFI:S:" + wifiManager->getConfigPortalSSID() +
                              ";T:WPA;P:" + softAP_password.c_str() + ";;";
        const String explainText = "*SSID: *\r\n" +
                                   wifiManager->getConfigPortalSSID() +
                                   "\r\n\r\n*Password:*\r\n" + softAP_password +
                                   "\r\n\r\n*Hostname*:\r\n" + getMyHostname();
        // Set the UNIX timestamp
        time_t timestamp = LAST_BUILD_TIME; // Example timestamp: March 7, 2021 00:00:00 UTC

        // Convert the timestamp to a struct tm in UTC
        struct tm *timeinfo = gmtime(&timestamp);

        // Format the date
        char formattedDate[20];
        strftime(formattedDate, sizeof(formattedDate), "%y-%m-%d\r\n%H:%M:%S", timeinfo);
        String hwStr = String(HW_REV);
        hwStr.replace("_EPD_", "\r\nEPD_");
        std::array<String, NUM_SCREENS> epdContent = {
            "Welcome!",
            "Bienvenidos!",
            "To setup\r\nscan QR or\r\nconnect\r\nmanually",
            "Para\r\nconfigurar\r\nescanear QR\r\no conectar\r\nmanualmente",
            explainText,
           "*HW version:*\r\n" + hwStr +
#ifdef GIT_TAG
            "\r\n\r\n*SW Version:*\r\n" + GIT_TAG +
#endif
            "\r\n\r\n*FW build date:*\r\n" + formattedDate,
            qrText};
      
        EPDManager::getInstance().setContent(epdContent); });

      wm.setSaveConfigCallback([]()
                               {
        preferences.putBool("wifiConfigured", true);

        delay(1000);
        // just restart after success
        ESP.restart(); });

      bool ac = wm.autoConnect(softAP_SSID.c_str(), softAP_password.c_str());
    }

    EPDManager::getInstance().setForegroundColor(preferences.getUInt("fgColor", isWhiteVersion() ? GxEPD_BLACK : GxEPD_WHITE));
    EPDManager::getInstance().setBackgroundColor(preferences.getUInt("bgColor", isWhiteVersion() ? GxEPD_WHITE : GxEPD_BLACK));
  }
  // else
  // {

  //   while (WiFi.status() != WL_CONNECTED)
  //   {
  //     vTaskDelay(pdMS_TO_TICKS(400));
  //   }
  // }
  // queueLedEffect(LED_EFFECT_WIFI_CONNECT_SUCCESS);
}

void syncTime()
{
  configTime(0, 0,
             NTP_SERVER);
  struct tm timeinfo;

  while (!getLocalTime(&timeinfo))
  {
    auto& ledHandler = getLedHandler();
    ledHandler.queueEffect(LED_EFFECT_CONFIGURING);
    configTime(0, 0,
               NTP_SERVER);
    delay(500);
    Serial.println(F("Retry set time"));
  }

  setTimezone(get_timezone_value_string(timezone_data::find_timezone_value(preferences.getString("tzString", DEFAULT_TZ_STRING))));

  lastTimeSync = esp_timer_get_time() / 1000000;
}

void setTimezone(String timezone) {
  Serial.printf("  Setting Timezone to %s\n",timezone.c_str());
  setenv("TZ",timezone.c_str(),1);  //  Now adjust the TZ.  Clock settings are adjusted to show the new local time
  tzset();
}


void setupPreferences()
{
  preferences.begin("btclock", false);

  EPDManager::getInstance().setForegroundColor(preferences.getUInt("fgColor", DEFAULT_FG_COLOR));
  EPDManager::getInstance().setBackgroundColor(preferences.getUInt("bgColor", DEFAULT_BG_COLOR));
  BlockNotify::getInstance().setBlockHeight(preferences.getUInt("blockHeight", INITIAL_BLOCK_HEIGHT));
  setPrice(preferences.getUInt("lastPrice", INITIAL_LAST_PRICE), CURRENCY_USD);

  if (!preferences.isKey("enableDebugLog")) {
    preferences.putBool("enableDebugLog", DEFAULT_ENABLE_DEBUG_LOG);
  }

  if (!preferences.isKey("dataSource")) {
    preferences.putUChar("dataSource", DEFAULT_DATA_SOURCE);
  }

  // Initialize custom endpoint settings if not set
  if (!preferences.isKey("customEndpoint")) {
    preferences.putString("customEndpoint", DEFAULT_CUSTOM_ENDPOINT);
  }

  if (!preferences.isKey("customEndpointDisableSSL")) {
    preferences.putBool("customEndpointDisableSSL", DEFAULT_CUSTOM_ENDPOINT_DISABLE_SSL);
  }

  // Set currency based on data source
  DataSourceType dataSource = static_cast<DataSourceType>(preferences.getUChar("dataSource", DEFAULT_DATA_SOURCE));
  if (dataSource == BTCLOCK_SOURCE || dataSource == CUSTOM_SOURCE) {
    ScreenHandler::setCurrentCurrency(preferences.getUChar("lastCurrency", CURRENCY_USD));
  } else {
    ScreenHandler::setCurrentCurrency(CURRENCY_USD);
  }

  if (!preferences.isKey("flDisable")) {
    preferences.putBool("flDisable", isWhiteVersion() ? false : true);
  }

  if (!preferences.isKey("gitReleaseUrl")) {
    preferences.putString("gitReleaseUrl", DEFAULT_GIT_RELEASE_URL);
  }

  if (!preferences.isKey("fgColor")) {
    preferences.putUInt("fgColor", isWhiteVersion() ? GxEPD_BLACK : GxEPD_WHITE);
    preferences.putUInt("bgColor", isWhiteVersion() ? GxEPD_WHITE : GxEPD_BLACK);
  }
 

  addScreenMapping(SCREEN_BLOCK_HEIGHT, "Block Height");

  addScreenMapping(SCREEN_TIME, "Time");
  addScreenMapping(SCREEN_HALVING_COUNTDOWN, "Halving countdown");
  addScreenMapping(SCREEN_BLOCK_FEE_RATE, "Block Fee Rate");

  addScreenMapping(SCREEN_SATS_PER_CURRENCY, "Sats per dollar");
  addScreenMapping(SCREEN_BTC_TICKER, "Ticker");
  addScreenMapping(SCREEN_MARKET_CAP, "Market Cap");

  // addScreenMapping(SCREEN_SATS_PER_CURRENCY_USD, "Sats per USD");
  // addScreenMapping(SCREEN_BTC_TICKER_USD, "Ticker USD");
  // addScreenMapping(SCREEN_MARKET_CAP_USD, "Market Cap USD");

  // addScreenMapping(SCREEN_SATS_PER_CURRENCY_EUR, "Sats per EUR");
  // addScreenMapping(SCREEN_BTC_TICKER_EUR, "Ticker EUR");
  // addScreenMapping(SCREEN_MARKET_CAP_EUR, "Market Cap EUR");

  // screenNameMap[SCREEN_BLOCK_HEIGHT] = "Block Height";
  // screenNameMap[SCREEN_BLOCK_FEE_RATE] = "Block Fee Rate";
  // screenNameMap[SCREEN_SATS_PER_CURRENCY] = "Sats per dollar";
  // screenNameMap[SCREEN_BTC_TICKER] = "Ticker";
  // screenNameMap[SCREEN_TIME] = "Time";
  // screenNameMap[SCREEN_HALVING_COUNTDOWN] = "Halving countdown";
  // screenNameMap[SCREEN_MARKET_CAP] = "Market Cap";

  // addCurrencyMappings(getActiveCurrencies());

  if (preferences.getBool("bitaxeEnabled", DEFAULT_BITAXE_ENABLED))
  {
    addScreenMapping(SCREEN_BITAXE_HASHRATE, "Bitaxe Hashrate");
    addScreenMapping(SCREEN_BITAXE_BESTDIFF, "Bitaxe Best Difficulty");
  }

  if (preferences.getBool("miningPoolStats", DEFAULT_MINING_POOL_STATS_ENABLED))
  {
    addScreenMapping(SCREEN_MINING_POOL_STATS_HASHRATE, "Mining Pool Hashrate");
    if (MiningPoolStatsFetch::getInstance().getPool()->supportsDailyEarnings()) {
      addScreenMapping(SCREEN_MINING_POOL_STATS_EARNINGS, "Mining Pool Earnings");
    }
  }
}

String replaceAmbiguousChars(String input)
{
  const char *ambiguous = "1IlO0";
  const char *replacements = "LKQM8";

  for (int i = 0; i < strlen(ambiguous); i++)
  {
    input.replace(ambiguous[i], replacements[i]);
  }

  return input;
}

void setupWebsocketClients(void *pvParameters)
{
  DataSourceType dataSource = getDataSource();
  
  if (dataSource == BTCLOCK_SOURCE || dataSource == CUSTOM_SOURCE)
  {
    V2Notify::setupV2Notify();
  }
  else if (dataSource == THIRD_PARTY_SOURCE)
  {
    BlockNotify::getInstance().setup();
    setupPriceNotify();
  }

  vTaskDelete(NULL);
}

void setupTimers()
{
  xTaskCreate(setupTimeUpdateTimer, "setupTimeUpdateTimer", 2048, NULL,
              tskIDLE_PRIORITY, NULL);
  xTaskCreate(setupScreenRotateTimer, "setupScreenRotateTimer", 2048, NULL,
              tskIDLE_PRIORITY, NULL);
}

void finishSetup()
{
  auto& ledHandler = getLedHandler();
  if (preferences.getBool("ledStatus", DEFAULT_LED_STATUS))
  {
    ledHandler.restoreLedState();
  }
  else
  {
    ledHandler.clear();
  }
}

std::vector<ScreenMapping> getScreenNameMap() { return screenMappings; }

void setupMcp()
{
#ifdef IS_BTCLOCK_V8
  const int mcp1AddrPins[] = {MCP1_A0_PIN, MCP1_A1_PIN, MCP1_A2_PIN};
  const int mcp1AddrValues[] = {LOW, LOW, LOW};

  const int mcp2AddrPins[] = {MCP2_A0_PIN, MCP2_A1_PIN, MCP2_A2_PIN};
  const int mcp2AddrValues[] = {HIGH, LOW, LOW};

  pinMode(MCP_RESET_PIN, OUTPUT);
  digitalWrite(MCP_RESET_PIN, HIGH);

  for (int i = 0; i < 3; ++i)
  {
    pinMode(mcp1AddrPins[i], OUTPUT);
    digitalWrite(mcp1AddrPins[i], mcp1AddrValues[i]);

    pinMode(mcp2AddrPins[i], OUTPUT);
    digitalWrite(mcp2AddrPins[i], mcp2AddrValues[i]);
  }

  digitalWrite(MCP_RESET_PIN, LOW);
  delay(30);
  digitalWrite(MCP_RESET_PIN, HIGH);
#endif
}

void setupHardware()
{
  if (!LittleFS.begin(true))
  {
    Serial.println(F("An Error has occurred while mounting LittleFS"));
  }

  if (HW_REV == "REV_B_EPD_2_13" && !isWhiteVersion()) {
    Serial.println(F("Black Rev B"));
  }

  if (!LittleFS.open("/index.html.gz", "r"))
  {
    Serial.println(F("Error loading WebUI"));
  }

  // Initialize LED handler
  auto& ledHandler = getLedHandler();
  ledHandler.setup();

  WiFi.setHostname(getMyHostname().c_str());
  if (!psramInit())
  {
    Serial.println(F("PSRAM not available"));
  }

  setupMcp();

  Wire.begin(I2C_SDA_PIN, I2C_SCK_PIN, 400000);

  if (!mcp1.begin()) {
    Serial.println(F("Error MCP23017 1"));
  } else {
    pinMode(MCP_INT_PIN, INPUT_PULLUP);
    
    // Enable mirrored interrupts (both INTA and INTB pins signal any interrupt)
    if (!mcp1.mirrorInterrupts(true)) {
        Serial.println(F("Error setting up mirrored interrupts"));
    }

    // Configure all 4 button pins as inputs with pullups and interrupts
    for (int i = 0; i < 4; i++) {
        if (!mcp1.pinMode1(i, INPUT_PULLUP)) {
            Serial.printf("Error setting pin %d to input pull up\n", i);
        }
        // Enable interrupt on CHANGE for each pin
        if (!mcp1.enableInterrupt(i, CHANGE)) {
            Serial.printf("Error enabling interrupt for pin %d\n", i);
        }
    }

    // Set interrupt pins as open drain with active-low polarity
    if (!mcp1.setInterruptPolarity(2)) { // 2 = Open drain
        Serial.println(F("Error setting interrupt polarity"));
    }

    // Clear any pending interrupts
    mcp1.getInterruptCaptureRegister();
  }

#ifdef IS_HW_REV_B
  pinMode(39, INPUT_PULLDOWN);

#endif

#ifdef IS_BTCLOCK_V8
  if (!mcp2.begin())
  {
    Serial.println(F("Error MCP23017 2"));

    // while (1)
    //         ;
  }
#endif

#ifdef HAS_FRONTLIGHT
  // Initialize frontlight through LedHandler
  ledHandler.initializeFrontlight();

  Wire.beginTransmission(0x5C);
  byte error = Wire.endTransmission();

  if (error == 0)
  {
    Serial.println(F("Found BH1750"));
    hasLuxSensor = true;
    bh1750.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x5C);
  }
  else
  {
    Serial.println(F("BH1750 Not found"));
    hasLuxSensor = false;
  }
#endif
}

void WiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info)
{
  static bool first_connect = true;
  auto& ledHandler = getLedHandler();  // Get ledHandler reference once at the start

  Serial.printf("[WiFi-event] event: %d\n", event);

  switch (event)
  {
  case ARDUINO_EVENT_WIFI_READY:
    Serial.println(F("WiFi interface ready"));
    break;
  case ARDUINO_EVENT_WIFI_SCAN_DONE:
    Serial.println(F("Completed scan for access points"));
    break;
  case ARDUINO_EVENT_WIFI_STA_START:
    Serial.println(F("WiFi client started"));
    break;
  case ARDUINO_EVENT_WIFI_STA_STOP:
    Serial.println(F("WiFi clients stopped"));
    break;
  case ARDUINO_EVENT_WIFI_STA_CONNECTED:
    Serial.println(F("Connected to access point"));
    break;
  case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
  {
    if (!first_connect)
    {
      Serial.println(F("Disconnected from WiFi access point"));
      ledHandler.queueEffect(LED_EFFECT_WIFI_CONNECT_ERROR);
      uint8_t reason = info.wifi_sta_disconnected.reason;
      if (reason)
        Serial.printf("Disconnect reason: %s, ",
                      WiFi.disconnectReasonName((wifi_err_reason_t)reason));
    }
    break;
  }
  case ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE:
    Serial.println(F("Authentication mode of access point has changed"));
    break;
  case ARDUINO_EVENT_WIFI_STA_GOT_IP:
  {
    Serial.print("Obtained IP address: ");
    Serial.println(WiFi.localIP());
    if (!first_connect)
      ledHandler.queueEffect(LED_EFFECT_WIFI_CONNECT_SUCCESS);
    first_connect = false;
    break;
  }
  case ARDUINO_EVENT_WIFI_STA_LOST_IP:
    Serial.println(F("Lost IP address and IP address is reset to 0"));
    ledHandler.queueEffect(LED_EFFECT_WIFI_CONNECT_ERROR);
    WiFi.reconnect();
    break;
  case ARDUINO_EVENT_WIFI_AP_START:
    Serial.println(F("WiFi access point started"));
    break;
  case ARDUINO_EVENT_WIFI_AP_STOP:
    Serial.println(F("WiFi access point  stopped"));
    break;
  case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
    Serial.println(F("Client connected"));
    break;
  case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
    Serial.println(F("Client disconnected"));
    break;
  case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
    Serial.println(F("Assigned IP address to client"));
    break;
  case ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED:
    Serial.println(F("Received probe request"));
    break;
  case ARDUINO_EVENT_WIFI_AP_GOT_IP6:
    Serial.println(F("AP IPv6 is preferred"));
    break;
  case ARDUINO_EVENT_WIFI_STA_GOT_IP6:
    Serial.println(F("STA IPv6 is preferred"));
    break;
  default:
    break;
  }
}

String getMyHostname()
{
  uint8_t mac[6];
  // WiFi.macAddress(mac);
  esp_efuse_mac_get_default(mac);
  char hostname[15];
  String hostnamePrefix = preferences.getString("hostnamePrefix", DEFAULT_HOSTNAME_PREFIX);
  snprintf(hostname, sizeof(hostname), "%s-%02x%02x%02x", hostnamePrefix,
           mac[3], mac[4], mac[5]);
  return hostname;
}

uint getLastTimeSync()
{
  return lastTimeSync;
}

#ifdef HAS_FRONTLIGHT

float getLightLevel()
{
  return bh1750.readLightLevel();
}

bool hasLightLevel()
{
  return hasLuxSensor;
}
#endif

String getHwRev()
{
#ifndef HW_REV
  return "REV_0";
#else
  return HW_REV;
#endif
}

bool isWhiteVersion()
{
#ifdef IS_HW_REV_B
  pinMode(39, INPUT_PULLDOWN);
  return digitalRead(39);
#else
  return false;
#endif
}

String getFsRev()
{
  File fsHash = LittleFS.open("/fs_hash.txt", "r");
  if (!fsHash)
  {
    Serial.println(F("Error loading WebUI"));
  }

  String ret = fsHash.readString();
  fsHash.close();
  return ret;
}

int findScreenIndexByValue(int value)
{
  for (int i = 0; i < screenMappings.size(); i++)
  {
    if (screenMappings[i].value == value)
    {
      return i;
    }
  }
  return -1; // Return -1 if value is not found
}

std::vector<std::string> getAvailableCurrencies()
{
  return {CURRENCY_CODE_USD, CURRENCY_CODE_EUR, CURRENCY_CODE_GBP, CURRENCY_CODE_JPY, CURRENCY_CODE_AUD, CURRENCY_CODE_CAD};
}

std::vector<std::string> getActiveCurrencies()
{
  std::vector<std::string> result;

  // Convert Arduino String to std::string
  std::string stdString = preferences.getString("actCurrencies", DEFAULT_ACTIVE_CURRENCIES).c_str();

  // Use a stringstream to split the string
  std::stringstream ss(stdString);
  std::string item;

  // Split the string by comma and add each part to the vector
  while (std::getline(ss, item, ','))
  {
    result.push_back(item);
  }
  return result;
}

bool isActiveCurrency(std::string &currency)
{
  std::vector<std::string> ac = getActiveCurrencies();
  if (std::find(ac.begin(), ac.end(), currency) != ac.end())
  {
    return true;
  }
  return false;
}

const char* getFirmwareFilename() {
    if (HW_REV == "REV_B_EPD_2_13") {
        return "btclock_rev_b_213epd_firmware.bin";
    } else if (HW_REV == "REV_A_EPD_2_13") {
        return "lolin_s3_mini_213epd_firmware.bin";
    } else if (HW_REV == "REV_A_EPD_2_9") {
        return "lolin_s3_mini_29epd_firmware.bin";
    } else {
        return "";
    }
}

const char* getWebUiFilename() {
    if (HW_REV == "REV_B_EPD_2_13") {
        return "littlefs_8MB.bin";
    } else if (HW_REV == "REV_A_EPD_2_13") {
        return "littlefs_4MB.bin";
    } else if (HW_REV == "REV_A_EPD_2_9") {
        return "littlefs_4MB.bin";
    } else {
        return "littlefs_4MB.bin";
    }
}

bool debugLogEnabled()
{
  return preferences.getBool("enableDebugLog", DEFAULT_ENABLE_DEBUG_LOG);
}

DataSourceType getDataSource() {
  return static_cast<DataSourceType>(preferences.getUChar("dataSource", DEFAULT_DATA_SOURCE));
}

void setDataSource(DataSourceType source) {
  preferences.putUChar("dataSource", static_cast<uint8_t>(source));
}
