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

uint wifiLostConnection;
uint priceNotifyLostConnection = 0;
uint blockNotifyLostConnection = 0;

int64_t getUptime() {
    return esp_timer_get_time() / 1000000;
}

void handlePriceNotifyDisconnection() {
    if (priceNotifyLostConnection == 0) {
        priceNotifyLostConnection = getUptime();
        Serial.println(F("Lost price notification connection, trying to reconnect..."));
    }
    
    if ((getUptime() - priceNotifyLostConnection) > 300) { // 5 minutes timeout
        Serial.println(F("Price notification connection lost for 5 minutes, restarting handler..."));
        restartPriceNotify();
        priceNotifyLostConnection = 0;
    }
}

void handleBlockNotifyDisconnection() {
    if (blockNotifyLostConnection == 0) {
        blockNotifyLostConnection = getUptime();
        Serial.println(F("Lost block notification connection, trying to reconnect..."));
    }
    
    if ((getUptime() - blockNotifyLostConnection) > 300) { // 5 minutes timeout
        Serial.println(F("Block notification connection lost for 5 minutes, restarting handler..."));
        restartBlockNotify();
        blockNotifyLostConnection = 0;
    }
} 

void handleFrontlight() {
#ifdef HAS_FRONTLIGHT
  if (hasLightLevel() && preferences.getUInt("luxLightToggle", DEFAULT_LUX_LIGHT_TOGGLE) != 0) {
    uint lightLevel = getLightLevel();
    uint luxThreshold = preferences.getUInt("luxLightToggle", DEFAULT_LUX_LIGHT_TOGGLE);
    
    if (lightLevel <= 1 && preferences.getBool("flOffWhenDark", DEFAULT_FL_OFF_WHEN_DARK)) {
      if (frontlightIsOn()) frontlightFadeOutAll();
    } else if (lightLevel < luxThreshold && !frontlightIsOn()) {
      frontlightFadeInAll();
    } else if (frontlightIsOn() && lightLevel > luxThreshold) {
      frontlightFadeOutAll();
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
  int currentBlock = getBlockFetch();
  if (currentBlock != -1) {
    if (currentBlock != getBlockHeight()) {
      Serial.println(F("Detected stuck block height... restarting block handler."));
      restartBlockNotify();
    }
    setLastBlockUpdate(getUptime());
  }
}


void monitorDataConnections() {
  // Price notification monitoring
  if (getPriceNotifyInit() && !preferences.getBool("fetchEurPrice", DEFAULT_FETCH_EUR_PRICE) && !isPriceNotifyConnected()) {
    handlePriceNotifyDisconnection();
  } else if (priceNotifyLostConnection > 0 && isPriceNotifyConnected()) {
    priceNotifyLostConnection = 0;
  }

  // Block notification monitoring
  if (getBlockNotifyInit() && !isBlockNotifyConnected()) {
    handleBlockNotifyDisconnection();
  } else if (blockNotifyLostConnection > 0 && isBlockNotifyConnected()) {
    blockNotifyLostConnection = 0;
  }

  // Check for missed price updates
  if ((getLastPriceUpdate(CURRENCY_USD) - getUptime()) > (preferences.getUInt("minSecPriceUpd", DEFAULT_SECONDS_BETWEEN_PRICE_UPDATE) * 5)) {
    Serial.println(F("Detected 5 missed price updates... restarting price handler."));
    restartPriceNotify();
    priceNotifyLostConnection = 0;
  }

  // Check for missed blocks
  if ((getLastBlockUpdate() - getUptime()) > 45 * 60) {
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
      monitorDataConnections();

      if (getUptime() - getLastTimeSync() > 24 * 60 * 60) {
        Serial.println(F("Last time update is longer than 24 hours ago, sync again"));
        syncTime();
      }
    }

    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}