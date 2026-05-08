#pragma once

#include <Arduino.h>

#include "lib/system/shared.hpp"
#include "lib/system/timers.hpp"
#include "lib/ui/screen_handler.hpp"

// Track timing for each button. Only single-click is implemented; the
// double-click / long-press state that used to live here was never wired
// to a handler and has been removed.
struct ButtonState {
  TickType_t lastPressTime = 0;
  bool isPressed = false;
};

class ButtonHandler {
private:
  static const TickType_t debounceDelay = pdMS_TO_TICKS(50);

  static ButtonState buttonStates[4];
  static TaskHandle_t buttonTaskHandle;

  static void handleButtonPress(int buttonIndex);
  static void handleButtonRelease(int buttonIndex);
  static void handleSingleClick(int buttonIndex);

  static void buttonTask(void *pvParameters);

public:
  static void setup();
  static void IRAM_ATTR handleButtonInterrupt();
  static void suspendTask() {
    if (buttonTaskHandle != NULL)
      vTaskSuspend(buttonTaskHandle);
  }
  static void resumeTask() {
    if (buttonTaskHandle != NULL)
      vTaskResume(buttonTaskHandle);
  }

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
