#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <Adafruit_NeoPixel.h>
#include "shared.hpp"

#ifndef NEOPIXEL_PIN
#define NEOPIXEL_PIN 34
#endif
#ifndef NEOPIXEL_COUNT
#define NEOPIXEL_COUNT 4
#endif

typedef struct {
    int flashType;
} LedTaskParameters;

const int LED_FLASH_ERROR = 0;
const int LED_FLASH_SUCCESS = 1;
const int LED_FLASH_UPDATE = 2;
const int LED_FLASH_BLOCK_NOTIFY = 3;
const int LED_EFFECT_START_TIMER = 4;
const int LED_EFFECT_PAUSE_TIMER = 5;

extern TaskHandle_t ledTaskHandle;

void ledTask(void *pvParameters);
void setupLeds();
void setupLedTask();
void blinkDelay(int d, int times);
void blinkDelayColor(int d, int times, uint r, uint g, uint b);
void blinkDelayTwoColor(int d, int times, uint32_t c1, uint32_t c2);
void clearLeds();
QueueHandle_t getLedTaskQueue();
bool queueLedEffect(uint effect);
void setLights(int r, int g, int b);