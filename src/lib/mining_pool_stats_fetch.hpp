#pragma once

#include <Arduino.h>
#include <HTTPClient.h>

#include "lib/config.hpp"
#include "lib/shared.hpp"

extern TaskHandle_t miningPoolStatsFetchTaskHandle;

void setupMiningPoolStatsFetchTask();
void taskMiningPoolStatsFetch(void *pvParameters);

std::string getMiningPoolStatsHashRate();
// std::string getMiningPoolStatsBestDiff();