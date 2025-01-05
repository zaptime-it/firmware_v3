#pragma once

#include <Arduino.h>
#include <HTTPClient.h>
#include <utils.hpp>

#include "lib/config.hpp"
#include "lib/shared.hpp"

extern TaskHandle_t bitaxeFetchTaskHandle;

void setupBitaxeFetchTask();
void taskBitaxeFetch(void *pvParameters);

uint64_t getBitAxeHashRate();
uint64_t getBitaxeBestDiff();