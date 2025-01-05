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

// using namespace websockets;

void setupBlockNotify();

void onWebsocketBlockEvent(void *handler_args, esp_event_base_t base,
                      int32_t event_id, void *event_data);
void onWebsocketBlockMessage(esp_websocket_event_data_t *event_data);

void setBlockHeight(uint32_t newBlockHeight);
uint32_t getBlockHeight();

void setBlockMedianFee(uint16_t blockMedianFee);
uint16_t getBlockMedianFee();

bool isBlockNotifyConnected();
void stopBlockNotify();
void restartBlockNotify();

void processNewBlock(uint32_t newBlockHeight);
void processNewBlockFee(uint16_t newBlockFee);

bool getBlockNotifyInit();
uint32_t getLastBlockUpdate();
int getBlockFetch();
void setLastBlockUpdate(uint32_t lastUpdate);
