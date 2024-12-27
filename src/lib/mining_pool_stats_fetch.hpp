#pragma once

#include <Arduino.h>
#include <HTTPClient.h>
#include "mining_pool/pool_factory.hpp"

#include "lib/config.hpp"
#include "lib/shared.hpp"

extern TaskHandle_t miningPoolStatsFetchTaskHandle;

void setupMiningPoolStatsFetchTask();
void taskMiningPoolStatsFetch(void *pvParameters);

std::string getMiningPoolStatsHashRate();
int getMiningPoolStatsDailyEarnings();

std::unique_ptr<MiningPoolInterface>& getMiningPool();
LogoData getMiningPoolLogo();