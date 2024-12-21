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
