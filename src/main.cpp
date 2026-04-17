/*
 * Copyright 2023-2024 Djuri Baars
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "Arduino.h"
#include <WiFiManager.h>
#define WEBSERVER_H
#include "ESPAsyncWebServer.h"
#include "lib/config.hpp"
#include "lib/led_handler.hpp"
#include "lib/block_notify.hpp"
#include "lib/live_service.hpp"

uint wifiLostConnection;

int64_t getUptime() {
    return esp_timer_get_time() / 1000000;
}

void handleFrontlight() {
#ifdef HAS_FRONTLIGHT
  if (hasLightLevel() && preferences.getUInt("luxLightToggle", DEFAULT_LUX_LIGHT_TOGGLE) != 0) {
    uint lightLevel = getLightLevel();
    uint luxThreshold = preferences.getUInt("luxLightToggle", DEFAULT_LUX_LIGHT_TOGGLE);
    auto& ledHandler = getLedHandler();
    
    if (lightLevel <= 1 && preferences.getBool("flOffWhenDark", DEFAULT_FL_OFF_WHEN_DARK)) {
      if (ledHandler.frontlightIsOn()) ledHandler.frontlightFadeOutAll();
    } else if (lightLevel < luxThreshold && !ledHandler.frontlightIsOn()) {
      ledHandler.frontlightFadeInAll();
    } else if (ledHandler.frontlightIsOn() && lightLevel > luxThreshold) {
      ledHandler.frontlightFadeOutAll();
    }
  }
#endif
}

void checkWiFiConnection() {
  if (!WiFi.isConnected()) {
    if (!wifiLostConnection) {
      wifiLostConnection = getUptime();
      Serial.println(F("Lost WiFi connection, trying to reconnect..."));
    }
    if ((getUptime() - wifiLostConnection) > 600) {
      Serial.println(F("Still no connection after 10 minutes, restarting..."));
      delay(2000);
      ESP.restart();
    }
    WiFi.begin();
  } else if (wifiLostConnection) {
    wifiLostConnection = 0;
    Serial.println(F("Connection restored, reset timer."));
  }
}

void checkMissedBlocks() {
  Serial.println(F("Long time (45 min) since last block, checking if I missed anything..."));
  auto& blockNotify = BlockNotify::getInstance();
  int currentBlock = blockNotify.fetchLatestBlock();
  if (currentBlock != -1) {
    if (currentBlock != blockNotify.getBlockHeight()) {
      Serial.println(F("Detected stuck block height... restarting block handler."));
      blockNotify.restart();
    }
    blockNotify.setLastBlockUpdate(getUptime());
  }
}

void monitorDataConnections() {
  // Delegate generic disconnect/staleness watchdogs to the registry.
  // Each LiveService declares its own staleAfterSeconds() policy, so
  // the bespoke "5 missed price updates" + "45 min no block" timers
  // are now expressed declaratively by PriceNotifyService and
  // BlockNotify respectively.
  LiveServiceRegistry::instance().monitor();

  // BlockNotify still gets a special-case REST probe on long silences,
  // because a live WebSocket can appear healthy while the upstream has
  // silently dropped new-block events.
  auto& blockNotify = BlockNotify::getInstance();
  int64_t uptimeNow = getUptime();
  int64_t lastBlockUpdate = static_cast<int64_t>(blockNotify.getLastBlockUpdate());
  if (blockNotify.isInitialized() && lastBlockUpdate != 0 &&
      uptimeNow > lastBlockUpdate &&
      (uptimeNow - lastBlockUpdate) > (45LL * 60LL)) {
    checkMissedBlocks();
  }
}

extern "C" void app_main() {
  initArduino();
  Serial.begin(115200);
  setup();

  while (true) {
    if (eventSourceTaskHandle != NULL) {
      xTaskNotifyGive(eventSourceTaskHandle);
    }

    if (!getIsOTAUpdating()) {
      handleFrontlight();
      checkWiFiConnection();

      // monitorDataConnections() now iterates the LiveServiceRegistry
      // so every active data source (BTCLOCK/CUSTOM V2, Mempool/Kraken,
      // Nostr) gets the same disconnect + staleness watchdog treatment.
      monitorDataConnections();

      if (getUptime() - getLastTimeSync() > 24 * 60 * 60) {
        Serial.println(F("Last time update is longer than 24 hours ago, sync again"));
        syncTime();
      }
    }

    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}