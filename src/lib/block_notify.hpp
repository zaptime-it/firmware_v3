#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <esp_timer.h>
#include <esp_websocket_client.h>
#include <cstring>
#include <string>

#include "lib/led_handler.hpp"
#include "lib/screen_handler.hpp"
#include "lib/timers.hpp"
#include "lib/shared.hpp"

class BlockNotify {
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
    void restart();
    bool isConnected() const;
    bool isInitialized() const;

    // Block height management
    void setBlockHeight(uint32_t newBlockHeight);
    uint32_t getBlockHeight() const;

    // Block fee management
    void setBlockMedianFee(uint16_t blockMedianFee);
    uint16_t getBlockMedianFee() const;

    // Block processing
    void processNewBlock(uint32_t newBlockHeight);
    void processNewBlockFee(uint16_t newBlockFee);

    // Block fetch and update tracking
    int fetchLatestBlock();
    uint getLastBlockUpdate() const;
    void setLastBlockUpdate(uint lastUpdate);

private:
    BlockNotify() = default;  // Private constructor for singleton
    
    void setupTask();
    static void onWebsocketEvent(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
    void onWebsocketMessage(esp_websocket_event_data_t *data);

    static const char* mempoolWsCert;
    static esp_websocket_client_handle_t wsClient;
    static uint32_t currentBlockHeight;
    static uint16_t blockMedianFee;
    static bool notifyInit;
    static unsigned long int lastBlockUpdate;
    static TaskHandle_t taskHandle;
};