#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WebSocketsClient.h>
#include <esp_timer.h>
#include <cstring>
#include <string>

#include "lib/drivers/leds/led_handler.hpp"
#include "lib/data_sources/live_service.hpp"
#include "lib/ui/screen_handler.hpp"
#include "lib/system/timers.hpp"
#include "lib/system/shared.hpp"

class BlockNotify : public LiveService {
public:
    static BlockNotify& getInstance() {
        static BlockNotify instance;
        return instance;
    }

    // Delete copy constructor and assignment operator
    BlockNotify(const BlockNotify&) = delete;
    void operator=(const BlockNotify&) = delete;

    // Block notification setup and control
    void setup();
    void stop();
    void restart() override;
    bool isConnected() const override;
    bool isInitialized() const override;

    // LiveService identity + watchdog policy
    const char* name() const override { return "BlockNotify"; }
    unsigned long lastUpdateSeconds() const override { return lastBlockUpdate; }
    // No new block in 45 minutes means something is wrong upstream. Aligns
    // with the ad-hoc "checkMissedBlocks" threshold from main.cpp.
    unsigned long staleAfterSeconds() const override { return 45UL * 60UL; }

    // Block height management
    void setBlockHeight(uint32_t newBlockHeight);
    uint32_t getBlockHeight() const;

    // Block fee management
    void setBlockMedianFee(float blockMedianFee);
    float getBlockMedianFee() const;

    // Block processing
    void processNewBlock(uint32_t newBlockHeight);
    void processNewBlockFee(float newBlockFee);

    // Block fetch and update tracking
    int fetchLatestBlock();
    uint getLastBlockUpdate() const;
    void setLastBlockUpdate(uint lastUpdate);

private:
    BlockNotify() = default;  // Private constructor for singleton

    static void onWebsocketEvent(WStype_t type, uint8_t *payload, size_t length);
    void onWebsocketMessage(uint8_t *payload, size_t length);
    static void taskBlockNotify(void *pvParameters);

    static WebSocketsClient wsClient;
    static uint32_t currentBlockHeight;
    static float blockMedianFee;
    static bool notifyInit;
    static bool wsConnected;
    static unsigned long int lastBlockUpdate;
    static TaskHandle_t taskHandle;
};