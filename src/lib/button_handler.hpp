#pragma once

#include <Arduino.h>

#include "lib/screen_handler.hpp"
#include "lib/shared.hpp"
#include "lib/timers.hpp"

extern TaskHandle_t buttonTaskHandle;

// Task and setup functions
void buttonTask(void *pvParameters);
void IRAM_ATTR handleButtonInterrupt();
void setupButtonTask();

// Individual button handlers
void handleButton1();
void handleButton2();
void handleButton3();
void handleButton4();

// New features
const TickType_t debounceDelay = pdMS_TO_TICKS(50);
const TickType_t doubleClickDelay = pdMS_TO_TICKS(300);  // Maximum time between clicks for double click
const TickType_t longPressDelay = pdMS_TO_TICKS(1000);   // Time to hold for long press

// Track timing for each button
struct ButtonState {
    TickType_t lastPressTime = 0;
    TickType_t pressStartTime = 0;
    bool isPressed = false;
    uint8_t clickCount = 0;
    bool longPressHandled = false;
};

extern ButtonState buttonStates[4];

#ifdef IS_BTCLOCK_V8
#define BTN_1 256
#define BTN_2 512
#define BTN_3 1024
#define BTN_4 2048
#else
#define BTN_1 2048
#define BTN_2 1024
#define BTN_3 512
#define BTN_4 256
#endif

void handleButtonPress(int buttonIndex);
void handleButtonRelease(int buttonIndex);
void handleSingleClick(int buttonIndex);
void handleDoubleClick(int buttonIndex);
void handleLongPress(int buttonIndex);
