#pragma once

#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <memory>

#include "lib/shared.hpp"
#include "lib/webserver.hpp"

#ifndef NEOPIXEL_PIN
#define NEOPIXEL_PIN 34
#endif
#ifndef NEOPIXEL_COUNT
#define NEOPIXEL_COUNT 4
#endif

// LED effect constants
const int LED_FLASH_ERROR = 0;
const int LED_FLASH_SUCCESS = 1;
const int LED_FLASH_UPDATE = 2;
const int LED_EFFECT_CONFIGURING = 10;
const int LED_FLASH_BLOCK_NOTIFY = 4;
const int LED_EFFECT_START_TIMER = 5;
const int LED_EFFECT_PAUSE_TIMER = 6;
const int LED_EFFECT_HEARTBEAT = 7;
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

// Do Not Disturb mode settings
struct DNDTimeRange {
    uint8_t startHour;
    uint8_t startMinute;
    uint8_t endHour;
    uint8_t endMinute;
};

class LedHandler {
public:
    static LedHandler& getInstance();
    
    // Delete copy constructor and assignment operator
    LedHandler(const LedHandler&) = delete;
    LedHandler& operator=(const LedHandler&) = delete;

    void setup();
    void setupTask();
    bool queueEffect(uint effect);
    void clear();
    void setLights(int r, int g, int b);
    void setLights(uint32_t color);
    void saveLedState();
    void restoreLedState();
    QueueHandle_t getTaskQueue() const { return ledTaskQueue; }
    Adafruit_NeoPixel& getPixels() { return pixels; }

    // DND methods
    void setDNDEnabled(bool enabled);
    void setDNDTimeBasedEnabled(bool enabled);
    void setDNDTimeRange(uint8_t startHour, uint8_t startMinute, uint8_t endHour, uint8_t endMinute);
    bool isDNDActive() const;
    bool isTimeInDNDRange(uint8_t hour, uint8_t minute) const;
    
    // DND getters
    bool isDNDEnabled() const { return dndEnabled; }
    bool isDNDTimeBasedEnabled() const { return dndTimeBasedEnabled; }
    uint8_t getDNDStartHour() const { return dndTimeRange.startHour; }
    uint8_t getDNDStartMinute() const { return dndTimeRange.startMinute; }
    uint8_t getDNDEndHour() const { return dndTimeRange.endHour; }
    uint8_t getDNDEndMinute() const { return dndTimeRange.endMinute; }

    // Effect methods
    void rainbow(int wait);
    void theaterChase(uint32_t color, int wait);
    void theaterChaseRainbow(int wait);
    void lightningStrike();
    void blinkDelay(int d, int times);
    void blinkDelayColor(int d, int times, uint r, uint g, uint b);
    void blinkDelayTwoColor(int d, int times, const uint32_t& c1, const uint32_t& c2);

#ifdef HAS_FRONTLIGHT
    void frontlightFlash(int flDelayTime);
    void frontlightFadeInAll();
    void frontlightFadeOutAll();
    void frontlightFadeIn(uint num);
    void frontlightFadeOut(uint num);
    std::vector<uint16_t> frontlightGetStatus();
    void frontlightSetBrightness(uint brightness);
    bool frontlightIsOn() const { return frontlightOn; }
    void frontlightFadeInAll(int flDelayTime, bool staggered = false);
    void frontlightFadeOutAll(int flDelayTime, bool staggered = false);
    void frontlightFadeIn(uint num, int flDelayTime);
    void frontlightFadeOut(uint num, int flDelayTime);
    void initializeFrontlight();
#endif

private:
    LedHandler();  // Private constructor for singleton
    void loadDNDSettings();
    static void ledTask(void* pvParameters);

    Adafruit_NeoPixel pixels;
    TaskHandle_t ledTaskHandle;
    QueueHandle_t ledTaskQueue;
    uint ledTaskParams;

    // DND members
    bool dndEnabled;
    bool dndTimeBasedEnabled;
    DNDTimeRange dndTimeRange;

#ifdef HAS_FRONTLIGHT
    static constexpr uint16_t FL_FADE_STEP = 25;
    bool frontlightOn;
    bool flInTransition;
#endif
};

// Global accessor function
inline LedHandler& getLedHandler() {
    return LedHandler::getInstance();
}