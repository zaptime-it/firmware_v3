#pragma once

#include <Arduino.h>

#include "lib/screen_handler.hpp"
#include "lib/shared.hpp"
#include "lib/timers.hpp"

// Track timing for each button
struct ButtonState {
    TickType_t lastPressTime = 0;
    TickType_t pressStartTime = 0;
    bool isPressed = false;
    uint8_t clickCount = 0;
    bool longPressHandled = false;
};

class ButtonHandler {
private:
    static const TickType_t debounceDelay = pdMS_TO_TICKS(50);
    static const TickType_t doubleClickDelay = pdMS_TO_TICKS(1000);  // Maximum time between clicks for double click
    static const TickType_t longPressDelay = pdMS_TO_TICKS(1500);   // Time to hold for long press

    static ButtonState buttonStates[4];
    static TaskHandle_t buttonTaskHandle;

    // Button handlers
    static void handleButtonPress(int buttonIndex);
    static void handleButtonRelease(int buttonIndex);
    static void handleSingleClick(int buttonIndex);
    static void handleDoubleClick(int buttonIndex);
    static void handleLongPress(int buttonIndex);

    // Task function
    static void buttonTask(void *pvParameters);

public:
    static void setup();
    static void IRAM_ATTR handleButtonInterrupt();
    static void suspendTask() { if (buttonTaskHandle != NULL) vTaskSuspend(buttonTaskHandle); }
    static void resumeTask() { if (buttonTaskHandle != NULL) vTaskResume(buttonTaskHandle); }

    #ifdef IS_BTCLOCK_V8
    static const uint16_t BTN_1 = 256;
    static const uint16_t BTN_2 = 512;
    static const uint16_t BTN_3 = 1024;
    static const uint16_t BTN_4 = 2048;
    #else
    static const uint16_t BTN_1 = 2048;
    static const uint16_t BTN_2 = 1024;
    static const uint16_t BTN_3 = 512;
    static const uint16_t BTN_4 = 256;
    #endif
};
