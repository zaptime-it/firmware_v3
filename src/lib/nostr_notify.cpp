#include "nostr_notify.hpp"

std::vector<nostr::NostrPool *> pools;
nostr::Transport *transport;
TaskHandle_t nostrTaskHandle = NULL;
boolean nostrIsConnected = false;
boolean nostrIsSubscribed = false;
boolean nostrIsSubscribing = true;

String subIdZap;

void setupNostrNotify(bool asDatasource, bool zapNotify)
{
    nostr::esp32::ESP32Platform::initNostr(false);
    // time_t now;
    // time(&now);
    // struct tm *utcTimeInfo;
    // utcTimeInfo = gmtime(&now);
    // time_t utcNow = mktime(utcTimeInfo);
    // time_t timestamp60MinutesAgo = utcNow - 3600;

    try
    {
        transport = nostr::esp32::ESP32Platform::getTransport();
        nostr::NostrPool *pool = new nostr::NostrPool(transport);
        String relay = preferences.getString("nostrRelay");
        String pubKey = preferences.getString("nostrPubKey");
        pools.push_back(pool);

        std::vector<nostr::NostrRelay *> *relays = pool->getConnectedRelays();
       
        if (zapNotify)
        {
            subscribeZaps(pool, relay, 60);
        }

        if (asDatasource)
        {
            String subId = pool->subscribeMany(
                {relay},
                {// First filter
                 {
                     {"kinds", {"1"}},
                     {"since", {String(getMinutesAgo(60))}},
                     {"authors", {pubKey}},
                 }},
                handleNostrEventCallback,
                onNostrSubscriptionClosed,
                onNostrSubscriptionEose
                );

            Serial.println(F("[ Nostr ] Subscribing to Nostr Data Feed"));
        }

        for (nostr::NostrRelay *relay : *relays)
        {
            Serial.println("[ Nostr ] Registering to connection events of: " + relay->getUrl());
            relay->getConnection()->addConnectionStatusListener([](const nostr::ConnectionStatus &status)
            { 
                static const char* STATUS_STRINGS[] = {"UNKNOWN", "CONNECTED", "DISCONNECTED", "ERROR"};
                int statusIndex = static_cast<int>(status);
                
                nostrIsConnected = (status == nostr::ConnectionStatus::CONNECTED);
                if (!nostrIsConnected) {
                    nostrIsSubscribed = false;
                }
                
                Serial.println("[ Nostr ] Connection status changed: " + String(STATUS_STRINGS[statusIndex])); 
            });
        }

    }
    catch (const std::exception &e)
    {
        Serial.println("[ Nostr ] Error: " + String(e.what()));
    }
}

void nostrTask(void *pvParameters)
{
    if(preferences.getBool("useNostr", DEFAULT_USE_NOSTR)) {
        int blockFetch = getBlockFetch();
        processNewBlock(blockFetch);
    }

    while (1)
    {
        for (nostr::NostrPool *pool : pools)
        {
            // Run internal loop: refresh relays, complete pending connections, send
            // pending messages
            pool->loop();
            if (!nostrIsSubscribed && !nostrIsSubscribing) {
                Serial.println(F("Not subscribed"));
                subscribeZaps(pool, preferences.getString("nostrRelay"), 1);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void setupNostrTask()
{
    xTaskCreate(nostrTask, "nostrTask", 8192, NULL, 10, &nostrTaskHandle);
}

boolean nostrConnected()
{
    return nostrIsConnected;
}

void onNostrSubscriptionClosed(const String &subId, const String &reason)
{
    // This is the callback that will be called when the subscription is
    // closed
    Serial.println("[ Nostr ] Subscription closed: " + reason);
}

void onNostrSubscriptionEose(const String &subId)
{
    // This is the callback that will be called when the subscription is
    // EOSE
    Serial.println("[ Nostr ] Subscription EOSE: " + subId);
    nostrIsSubscribing = false;
    nostrIsSubscribed = true;
}

void handleNostrEventCallback(const String &subId, nostr::SignedNostrEvent *event)
{
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

    // Use direct value access instead of multiple comparisons
    String typeValue;
    uint medianFee = 0;
    
    for (JsonArray tag : tags) {
        if (tag.size() != 2) continue;
        
        const char *key = tag[0];
        if (!key) continue;
        
        // Use switch for better performance on string comparisons
        switch (key[0]) {
            case 't':  // type
                if (strcmp(key, "type") == 0) {
                    const char *value = tag[1];
                    if (value) typeValue = value;
                }
                break;
            case 'm':  // medianFee
                if (strcmp(key, "medianFee") == 0) {
                    medianFee = tag[1].as<uint>();
                }
                break;
        }
    }
    
    // Process the data
    if (!typeValue.isEmpty()) {
        if (typeValue == "priceUsd") {
            processNewPrice(obj["content"].as<uint>(), CURRENCY_USD);
        }
        else if (typeValue == "blockHeight") {
            processNewBlock(obj["content"].as<uint>());
        }

        if (medianFee != 0) {
            processNewBlockFee(medianFee);
        }
    }
}

time_t getMinutesAgo(int min) {
    time_t now;
    time(&now);
    return now - (min * 60);
}

void subscribeZaps(nostr::NostrPool *pool, const String &relay, int minutesAgo) {
    if (subIdZap) {
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
    Serial.println("[ Nostr ] Subscribing to Zap Notifications since " + String(getMinutesAgo(minutesAgo)));
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

    Serial.printf("Got a zap of %llu sats for %s\n", zapAmount, zapPubkey.c_str());

    uint64_t timerPeriod = 0;
    if (isTimerActive())
    {
        // store timer periode before making inactive to prevent artifacts
        timerPeriod = getTimerSeconds();
        esp_timer_stop(screenRotateTimer);
    }
    setCurrentScreen(SCREEN_CUSTOM);

    setEpdContent(textEpdContent);
    vTaskDelay(pdMS_TO_TICKS(315 * NUM_SCREENS) + pdMS_TO_TICKS(250));
    if (preferences.getBool("ledFlashOnZap", DEFAULT_LED_FLASH_ON_ZAP))
    {
        queueLedEffect(LED_EFFECT_NOSTR_ZAP);
    }
    if (timerPeriod > 0)
    {
        esp_timer_start_periodic(screenRotateTimer,
                                timerPeriod * usPerSecond);
    }
}