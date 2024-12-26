#pragma once

#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "lib/shared.hpp"
#include "lib/webserver.hpp"

#ifndef NEOPIXEL_PIN
#define NEOPIXEL_PIN 34
#endif
#ifndef NEOPIXEL_COUNT
#define NEOPIXEL_COUNT 4
#endif

const int LED_FLASH_ERROR = 0;
const int LED_FLASH_SUCCESS = 1;
const int LED_FLASH_UPDATE = 2;
const int LED_FLASH_BLOCK_NOTIFY = 3;
const int LED_EFFECT_START_TIMER = 4;
const int LED_EFFECT_PAUSE_TIMER = 5;
const int LED_EFFECT_HEARTBEAT = 6;
const int LED_EFFECT_WIFI_WAIT_FOR_CONFIG = 100;
const int LED_EFFECT_WIFI_CONNECTING = 101;
const int LED_EFFECT_WIFI_CONNECT_ERROR = 102;
const int LED_EFFECT_WIFI_CONNECT_SUCCESS = 103;
const int LED_EFFECT_WIFI_ERASE_SETTINGS = 104;


const int LED_PROGRESS_25 = 200;
const int LED_PROGRESS_50 = 201;
const int LED_PROGRESS_75 = 202;
const int LED_PROGRESS_100 = 203;

const int LED_DATA_PRICE_ERROR = 300;
const int LED_DATA_BLOCK_ERROR = 301;

const int LED_EFFECT_NOSTR_ZAP = 400;

const int LED_FLASH_IDENTIFY = 990;
const int LED_POWER_TEST = 999;
extern TaskHandle_t ledTaskHandle;
extern Adafruit_NeoPixel pixels;

void ledTask(void *pvParameters);
void setupLeds();
void setupLedTask();
void blinkDelay(int d, int times);
void blinkDelayColor(int d, int times, uint r, uint g, uint b);
void blinkDelayTwoColor(int d, int times, const uint32_t& c1, const uint32_t& c2);
void clearLeds();
void saveLedState();
void restoreLedState();
QueueHandle_t getLedTaskQueue();
bool queueLedEffect(uint effect);
void setLights(int r, int g, int b);
void setLights(uint32_t color);
void ledRainbow(int wait);
void ledTheaterChaseRainbow(int wait);
void ledTheaterChase(uint32_t color, int wait);
Adafruit_NeoPixel getPixels();
void lightningStrike();

#ifdef HAS_FRONTLIGHT
void frontlightFlash(int flDelayTime);
void frontlightFadeInAll();
void frontlightFadeOutAll();
void frontlightFadeIn(uint num);
void frontlightFadeOut(uint num);

std::vector<uint16_t> frontlightGetStatus();

void frontlightSetBrightness(uint brightness);
bool frontlightIsOn();

void frontlightFadeInAll(int flDelayTime);
void frontlightFadeInAll(int flDelayTime, bool staggered);
void frontlightFadeOutAll(int flDelayTime);
void frontlightFadeOutAll(int flDelayTime, bool staggered);

void frontlightFadeIn(uint num, int flDelayTime);
void frontlightFadeOut(uint num, int flDelayTime);
#endif