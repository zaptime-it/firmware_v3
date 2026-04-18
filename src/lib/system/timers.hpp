#pragma once

#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "lib/system/shared.hpp"
#include "lib/ui/screen_handler.hpp"

extern esp_timer_handle_t screenRotateTimer;
extern esp_timer_handle_t minuteTimer;

void setupTimeUpdateTimer(void *pvParameters);
void setupScreenRotateTimer(void *pvParameters);

void IRAM_ATTR minuteTimerISR(void *arg);
void IRAM_ATTR screenRotateTimerISR(void *arg);

uint getTimerSeconds();
bool isTimerActive();
void setTimerActive(bool status);
void toggleTimerActive();

// ISR-safe task-handle registration. The minute timer ISR runs from IRAM
// and must not reach into flash-resident singleton accessors, so the
// relevant task handles are cached in module-level volatile statics set
// once at task creation time.
void setBitaxeTaskHandleForIsr(TaskHandle_t handle);
void setMiningPoolTaskHandleForIsr(TaskHandle_t handle);