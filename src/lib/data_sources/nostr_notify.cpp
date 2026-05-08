#include "nostr_notify.hpp"
#include "lib/drivers/leds/led_handler.hpp"
#include "lib/system/config.hpp"
#include "lib/system/tls_gate.hpp"

#include <cstdlib>
#include <mutex>
#include <new>

std::vector<nostr::NostrPool *> pools;
nostr::Transport *transport;
TaskHandle_t nostrTaskHandle = NULL;
boolean nostrIsConnected = false;
boolean nostrIsSubscribed = false;
boolean nostrIsSubscribing = true;
static bool s_nostrAsDatasource = false;
static bool s_nostrZapNotify = false;
static unsigned long s_lastNostrUpdate = 0;

String subIdZap;
static String subIdData;

static bool parseUintFromString(const char *s, uint &out)
{
    if (s == nullptr || *s == '\0') return false;
    char *end = nullptr;
    unsigned long v = strtoul(s, &end, 10);
    if (end == s) return false;
    out = static_cast<uint>(v);
    return true;
}

static bool parseFloatFromString(const char *s, float &out)
{
    if (s == nullptr || *s == '\0') return false;
    char *end = nullptr;
    float v = strtof(s, &end);
    if (end == s) return false;
    out = v;
    return true;
}

bool nostrIsInitialized() { return s_nostrAsDatasource || s_nostrZapNotify; }
unsigned long getLastNostrUpdate() { return s_lastNostrUpdate; }

/** nostr::ConnectionStatus is CONNECTED=0, DISCONNECTED=1, ERROR=2 — do not index a string array by raw int. */
static const char *nostrConnectionStatusName(nostr::ConnectionStatus status)
{
    switch (status) {
    case nostr::ConnectionStatus::CONNECTED:
        return "CONNECTED";
    case nostr::ConnectionStatus::DISCONNECTED:
        return "DISCONNECTED";
    case nostr::ConnectionStatus::ERROR:
        return "ERROR";
    default:
        return "UNKNOWN";
    }
}

void screenRestoreAfterZapCallback(TimerHandle_t xTimer)
{
    int screenBeforeZap = (int)(uintptr_t)pvTimerGetTimerID(xTimer);
    ScreenHandler::setCurrentScreen(screenBeforeZap);
    xTimerDelete(xTimer, 0);
}

void setupNostrNotify(bool asDatasource, bool zapNotify)
{
    s_nostrAsDatasource = asDatasource;
    s_nostrZapNotify = zapNotify;
    nostr::esp32::ESP32Platform::initNostr(false);
    // time_t now;
    // time(&now);
    // struct tm *utcTimeInfo;
    // utcTimeInfo = gmtime(&now);
    // time_t utcNow = mktime(utcTimeInfo);
    // time_t timestamp60MinutesAgo = utcNow - 3600;

    transport = nostr::esp32::ESP32Platform::getTransport();
    if (transport == nullptr) {
        return;
    }
    nostr::NostrPool *pool = new (std::nothrow) nostr::NostrPool(transport);
    if (pool == nullptr) {
        return;
    }
    // Always pass the build-time default for these two keys. Historically
    // they were read without a default, so any NVS miss (fresh install,
    // corrupted entry, transient read failure) produced an empty URL that
    // still went through ensureRelay → NostrRelay("", …). The resulting
    // ghost relay never connects but happily fires DISCONNECTED events
    // that flip nostrIsConnected to false, making the WebUI's Nostr
    // status column stick at "disconnected" forever even though the real
    // primal.net relay is up.
    String relay = preferences.getString("nostrRelay", DEFAULT_NOSTR_RELAY);
    String pubKey = preferences.getString("nostrPubKey", DEFAULT_NOSTR_NPUB);
    if (relay.length() == 0) {
        // Belt and braces: if the default is ever set to "" we still
        // refuse to create a ghost relay.
        Serial.println(F("[ Nostr ] skipping setup: relay URL is empty"));
        return;
    }
    pools.push_back(pool);

    std::vector<nostr::NostrRelay *> *relays = pool->getConnectedRelays();

    if (zapNotify)
    {
        subscribeZaps(pool, relay, 60);
    }

    if (asDatasource)
    {
        // Prefer the ws-node Nostr publisher format:
        // - kind 30078 (parameterized-replaceable, NIP-78)
        // - `d` tag slot per datum: price:<CCY>, blockheight, medianFee
        // - content carries the value as a string
        subIdData = pool->subscribeMany(
            {relay},
            {// First filter
             {
                 {"kinds", {"30078"}},
                 {"since", {String(getMinutesAgo(60))}},
                 {"authors", {pubKey}},
             }},
            handleNostrEventCallback,
            onNostrSubscriptionClosed,
            onNostrSubscriptionEose);

        if (debugLogEnabled())
        {
            Serial.printf("[ Nostr ] debug: data subscription subId=%s relay=%s kinds=30078 since=%s author=%s\n",
                          subIdData.c_str(), relay.c_str(), String(getMinutesAgo(60)).c_str(), pubKey.c_str());
        }
    }

    if (relays == nullptr) {
        return;
    }

    for (nostr::NostrRelay *r : *relays)
    {
        // Skip any relay that somehow ended up with an empty URL. Such
        // an entry never completes the TCP connect, so its listener
        // only ever sees DISCONNECTED — which would overwrite
        // nostrIsConnected set to true by the real relay.
        if (r == nullptr || r->getUrl().length() == 0) continue;

        r->getConnection()->addConnectionStatusListener([r](const nostr::ConnectionStatus &status)
        {
            nostrIsConnected = (status == nostr::ConnectionStatus::CONNECTED);
            if (!nostrIsConnected) {
                nostrIsSubscribed = false;
            }
            if (debugLogEnabled())
            {
                Serial.printf("[ Nostr ] debug: relay %s connection status=%s\n",
                              r->getUrl().c_str(), nostrConnectionStatusName(status));
            }
        });
    }
}

void nostrTask(void *pvParameters)
{
    DataSourceType dataSource = getDataSource();
    if(dataSource == NOSTR_SOURCE) {
        auto& blockNotify = BlockNotify::getInstance();
        int blockFetch = blockNotify.fetchLatestBlock();
        blockNotify.processNewBlock(blockFetch);
    }

    while (1)
    {
        for (nostr::NostrPool *pool : pools)
        {
            // Run internal loop: refresh relays, complete pending connections, send
            // pending messages. Take the firmware-wide TLS gate only while
            // we are not known-connected — same pattern as the other WS
            // clients, so the Nostr TLS handshake doesn't stack on top of
            // the mempool/Kraken/HTTP handshakes at boot or after a
            // network flap.
            if (nostrIsConnected) {
                pool->loop();
            } else {
                std::lock_guard<std::mutex> lk(tls_gate::mutex());
                pool->loop();
            }
            if (!nostrIsSubscribed && !nostrIsSubscribing) {
                if (debugLogEnabled())
                {
                }
                subscribeZaps(pool, preferences.getString("nostrRelay", DEFAULT_NOSTR_RELAY), 1);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void setupNostrTask()
{
    xTaskCreate(nostrTask, "nostrTask", 8192, NULL, 10, &nostrTaskHandle);
}

void stopNostrNotify()
{
    TaskHandle_t caller = xTaskGetCurrentTaskHandle();
    if (nostrTaskHandle != NULL && nostrTaskHandle != caller)
    {
        vTaskDelete(nostrTaskHandle);
        nostrTaskHandle = NULL;
    }
    for (nostr::NostrPool *pool : pools)
    {
        delete pool;
    }
    pools.clear();
    nostrIsConnected = false;
    nostrIsSubscribed = false;
    nostrIsSubscribing = true;
    s_nostrAsDatasource = false;
    s_nostrZapNotify = false;
}

void restartNostrNotify()
{
    bool wasDatasource = s_nostrAsDatasource;
    bool wasZap = s_nostrZapNotify;
    stopNostrNotify();
    if (wasDatasource || wasZap) {
        setupNostrNotify(wasDatasource, wasZap);
        setupNostrTask();
    }
}

boolean nostrConnected()
{
    return nostrIsConnected;
}

void onNostrSubscriptionClosed(const String &subId, const String &reason)
{
    // This is the callback that will be called when the subscription is
    // closed
    if (debugLogEnabled())
    {
    }
}

void onNostrSubscriptionEose(const String &subId)
{
    // This is the callback that will be called when the subscription is
    // EOSE
    nostrIsSubscribing = false;
    nostrIsSubscribed = true;
}

void handleNostrEventCallback(const String &subId, nostr::SignedNostrEvent *event)
{
    s_lastNostrUpdate = static_cast<unsigned long>(esp_timer_get_time() / 1000000);
    JsonDocument doc;
    JsonArray arr = doc["data"].to<JsonArray>();
    event->toSendableEvent(arr);
    
    // Early return if array is invalid
    if (arr.size() < 2 || !arr[1].is<JsonObject>()) {
        return;
    }

    JsonObject obj = arr[1].as<JsonObject>();
    JsonArray tags = obj["tags"].as<JsonArray>();
    if (!tags) {
        return;
    }

    // ws-node publisher format (kind 30078): tag ["d","slot"], content holds the value
    String dTag;
    
    for (JsonArray tag : tags) {
        if (tag.size() != 2) continue;
        
        const char *key = tag[0];
        if (!key) continue;
        
        // Use switch for better performance on string comparisons
        switch (key[0]) {
            case 'd':  // d tag (parameterized-replaceable slot)
                if (strcmp(key, "d") == 0) {
                    const char *value = tag[1];
                    if (value) dTag = value;
                }
                break;
        }
    }
    
    const char *contentStr = obj["content"].as<const char *>();
    if (dTag.isEmpty()) return;

    auto &blockNotify = BlockNotify::getInstance();
    if (dTag == "blockheight")
    {
        uint h = 0;
        if (parseUintFromString(contentStr, h)) {
            blockNotify.processNewBlock(h);
        }
        return;
    }
    if (dTag == "medianFee")
    {
        float fee = 0.0f;
        if (parseFloatFromString(contentStr, fee)) {
            blockNotify.processNewBlockFee(fee);
        }
        return;
    }
    if (dTag.startsWith("price:"))
    {
        String code = dTag.substring(6);
        if (code.length() > 0)
        {
            uint price = 0;
            if (parseUintFromString(contentStr, price)) {
                processNewPrice(price, getCurrencyChar(code.c_str()));
            }
        }
        return;
    }
}

time_t getMinutesAgo(int min) {
    time_t now;
    time(&now);
    return now - (min * 60);
}

void subscribeZaps(nostr::NostrPool *pool, const String &relay, int minutesAgo) {
    if (subIdZap) {
        if (debugLogEnabled())
        {
        }
        pool->closeSubscription(subIdZap);
    }
    nostrIsSubscribing = true;

    subIdZap = pool->subscribeMany(
        {relay},
        {
            {
                {"kinds", {"9735"}},
                {"limit", {"1"}},
                {"since", {String(getMinutesAgo(minutesAgo))}},
                {"#p", {preferences.getString("nostrZapPubkey", DEFAULT_ZAP_NOTIFY_PUBKEY)                }},
                //     {"#p", [&]() {
                //     std::initializer_list<NostrString> pubkeys;
                //     String pubkeysStr = preferences.getString("nostrZapPubkeys", "");
                //     if (pubkeysStr.length() > 0) {
                //         // Assuming pubkeys are comma-separated
                //         char* str = strdup(pubkeysStr.c_str());
                //         char* token = strtok(str, ",");
                //         std::vector<NostrString> keys;
                //         while (token != NULL) {
                //             keys.push_back(String(token));
                //             token = strtok(NULL, ",");
                //         }
                //         free(str);
                //         return std::initializer_list<NostrString>(keys.begin(), keys.end());
                //     }
                //     // Return default if no pubkeys found
                //     return std::initializer_list<NostrString>{
                //         preferences.getString("nostrZapPubkey", DEFAULT_ZAP_NOTIFY_PUBKEY)
                //     };
                // }()},
            },
        },
        handleNostrZapCallback,
        onNostrSubscriptionClosed,
        onNostrSubscriptionEose);
    if (debugLogEnabled())
    {
        String zapPubkey = preferences.getString("nostrZapPubkey", DEFAULT_ZAP_NOTIFY_PUBKEY);
        Serial.printf("[ Nostr ] debug: zap subscription subId=%s relay=%s kinds=9735 since=%s #p=%s\n",
                      subIdZap.c_str(), relay.c_str(), String(getMinutesAgo(minutesAgo)).c_str(), zapPubkey.c_str());
    }
}

void handleNostrZapCallback(const String &subId, nostr::SignedNostrEvent *event) {
    JsonDocument doc;
    JsonArray arr = doc["data"].to<JsonArray>();
    event->toSendableEvent(arr);
    
    // Early return if invalid
    if (arr.size() < 2 || !arr[1].is<JsonObject>()) {
        return;
    }

    JsonObject obj = arr[1].as<JsonObject>();
    JsonArray tags = obj["tags"].as<JsonArray>();
    if (!tags) {
        return;
    }

    uint64_t zapAmount = 0;
    String zapPubkey;
    
    for (JsonArray tag : tags) {
        if (tag.size() != 2) continue;
        
        const char *key = tag[0];
        const char *value = tag[1];
        if (!key || !value) continue;

        if (key[0] == 'b' && strcmp(key, "bolt11") == 0) {
            zapAmount = getAmountInSatoshis(std::string(value));
        } 
        else if (key[0] == 'p' && strcmp(key, "p") == 0) {
            zapPubkey = value;
        }
    }

    if (zapAmount == 0) return;
    
    std::array<std::string, NUM_SCREENS> textEpdContent = parseZapNotify(zapAmount, preferences.getBool("useSatsSymbol", DEFAULT_USE_SATS_SYMBOL));

    if (debugLogEnabled())  
    {
    }

    uint64_t timerPeriod = 0;
    int screenBeforeZap = ScreenHandler::getCurrentScreen();
    if (isTimerActive())
    {
        // store timer periode before making inactive to prevent artifacts
        timerPeriod = getTimerSeconds();
        esp_timer_stop(screenRotateTimer);
    } 
    ScreenHandler::setCurrentScreen(SCREEN_CUSTOM);

    EPDManager::getInstance().setContent(textEpdContent);
    vTaskDelay(pdMS_TO_TICKS(315 * NUM_SCREENS) + pdMS_TO_TICKS(250));
    if (preferences.getBool("ledFlashOnZap", DEFAULT_LED_FLASH_ON_ZAP))
    {
        getLedHandler().queueEffect(LED_EFFECT_NOSTR_ZAP);
    }
    if (timerPeriod > 0)
    {
        esp_timer_start_periodic(screenRotateTimer,
                                timerPeriod * usPerSecond);
    } else if (preferences.getBool("scrnRestoreZap", DEFAULT_SCREEN_RESTORE_AFTER_ZAP)) {
        TimerHandle_t screenRestoreAfterZapTimer = xTimerCreate(
            "screenRestoreAfterZap",
            pdMS_TO_TICKS(getTimerSeconds() * msPerSecond),
            pdFALSE,
            (void*)(uintptr_t)screenBeforeZap,
            screenRestoreAfterZapCallback);
        if (screenRestoreAfterZapTimer == nullptr) {
            return;
        }
        if (xTimerStart(screenRestoreAfterZapTimer, 0) != pdPASS) {
            // Tear down the timer on start failure to avoid leaking it; the
            // callback normally deletes itself but we never reached it.
            xTimerDelete(screenRestoreAfterZapTimer, 0);
        }
    }
}

// void onNostrEvent(const String &subId, const nostr::Event &event) {
//     // This is the callback that will be called when a new event is received
//     if (event.kind == 9735) {
//         // Parse the zap amount from the event
//         uint16_t amount = parseZapAmount(event);
//         if (amount > 0) {
//             std::array<std::string, NUM_SCREENS> zapContent = parseZapNotify(amount, true);
//             EPDManager::getInstance().setContent(zapContent);
            
//             if (preferences.getBool("ledFlashOnUpd", DEFAULT_LED_FLASH_ON_UPD)) {
//                 getLedHandler().queueEffect(LED_FLASH_BLOCK_NOTIFY);
//             }
//         }
//     }
// }