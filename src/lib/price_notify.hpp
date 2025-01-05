#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include <string>

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