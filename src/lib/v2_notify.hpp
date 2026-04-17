#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include "block_notify.hpp"
#include <string>
#include <memory>

#include "lib/live_service.hpp"
#include "lib/screen_handler.hpp"

namespace V2Notify {
    extern TaskHandle_t v2NotifyTaskHandle;

    void setupV2NotifyTask();
    void taskV2Notify(void *pvParameters);

    void restartV2Notify();
    void setupV2Notify();
    void stopV2Notify();
    void onWebsocketV2Event(WStype_t type, uint8_t * payload, size_t length);
    void handleV2Message(JsonDocument doc);

    bool isV2NotifyConnected();
    bool isV2NotifyInitialized();
    unsigned long getLastV2Update();
}

// Adapter exposing V2Notify through the LiveService interface so the
// registry-driven monitor treats it like any other live data feed.
class V2NotifyService : public LiveService {
public:
    static V2NotifyService& getInstance() {
        static V2NotifyService instance;
        return instance;
    }
    const char* name() const override { return "V2Notify"; }
    bool isInitialized() const override { return V2Notify::isV2NotifyInitialized(); }
    bool isConnected() const override { return V2Notify::isV2NotifyConnected(); }
    unsigned long lastUpdateSeconds() const override { return V2Notify::getLastV2Update(); }
    // V2 funnels price + block + fee into a single stream, so one minute of
    // silence on USD price is our staleness signal.
    unsigned long staleAfterSeconds() const override { return 5UL * 60UL; }
    void restart() override { V2Notify::restartV2Notify(); }
};