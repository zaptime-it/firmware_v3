#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include <string>

#include "lib/live_service.hpp"
#include "lib/screen_handler.hpp"

extern TaskHandle_t priceNotifyTaskHandle;

void setupPriceNotify();
void setupPriceNotifyTask();
void taskPriceNotify(void *pvParameters);

void onWebsocketPriceEvent(WStype_t type, uint8_t * payload, size_t length);

uint getPrice(char currency);
void setPrice(uint newPrice, char currency);

void processNewPrice(uint newPrice, char currency);

bool isPriceNotifyConnected();
void stopPriceNotify();
void restartPriceNotify();

bool getPriceNotifyInit();
uint getLastPriceUpdate(char currency);
void loadStoredPrices();

// Adapter exposing the free-function PriceNotify module through the
// LiveService interface so it can be driven by LiveServiceRegistry.
class PriceNotifyService : public LiveService {
public:
    static PriceNotifyService& getInstance() {
        static PriceNotifyService instance;
        return instance;
    }
    const char* name() const override { return "PriceNotify"; }
    bool isInitialized() const override { return getPriceNotifyInit(); }
    bool isConnected() const override { return isPriceNotifyConnected(); }
    unsigned long lastUpdateSeconds() const override;
    unsigned long staleAfterSeconds() const override;
    void restart() override { restartPriceNotify(); }
};