#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <esp_websocket_client.h>
#include "block_notify.hpp"
#include <string>

#include "lib/screen_handler.hpp"

void setupPriceNotify();

void onWebsocketPriceEvent(void *handler_args, esp_event_base_t base,
                           int32_t event_id, void *event_data);
//void onWebsocketPriceEvent(WStype_t type, uint8_t * payload, size_t length);

void onWebsocketPriceMessage(esp_websocket_event_data_t *event_data);

uint getPrice(char currency);
void setPrice(uint newPrice, char currency);

//void processNewPrice(uint newPrice);
void processNewPrice(uint newPrice, char currency);

bool isPriceNotifyConnected();
void stopPriceNotify();
void restartPriceNotify();

bool getPriceNotifyInit();
uint getLastPriceUpdate(char currency);
void loadStoredPrices();