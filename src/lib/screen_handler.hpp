#pragma once

#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <data_handler.hpp>
#include <bitaxe_handler.hpp>
#include "lib/mining_pool/mining_pool_stats_handler.hpp"

#include "lib/epd.hpp"
#include "lib/shared.hpp"

#define WORK_QUEUE_SIZE 10

extern TaskHandle_t workerTaskHandle;
extern TaskHandle_t taskScreenRotateTaskHandle;
extern QueueHandle_t workQueue;

typedef enum {
  TASK_PRICE_UPDATE,
  TASK_BLOCK_UPDATE,
  TASK_FEE_UPDATE,
  TASK_TIME_UPDATE,
  TASK_BITAXE_UPDATE,
  TASK_MINING_POOL_STATS_UPDATE
} TaskType;

typedef struct {
  TaskType type;
  char data;
} WorkItem;

class ScreenHandler {
private:
    static uint currentScreen;
    static uint currentCurrency;

public:
    static uint getCurrentScreen() { return currentScreen; }
    static uint getCurrentCurrency() { return currentCurrency; }
    static void setCurrentScreen(uint newScreen);
    static void setCurrentCurrency(char currency);
    static void nextScreen();
    static void previousScreen();
    static void showSystemStatusScreen();
    static bool isCurrencySpecific(uint screen);
    static bool handleCurrencyRotation(bool forward);
    static int findNextVisibleScreen(int currentScreen, bool forward);
};

// Keep as free functions since they deal with FreeRTOS tasks
void workerTask(void *pvParameters);
void taskScreenRotate(void *pvParameters);
void setupTasks();
void cleanup();