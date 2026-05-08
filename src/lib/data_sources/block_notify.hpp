#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WebSocketsClient.h>
#include <atomic>
#include <cstring>
#include <esp_timer.h>
#include <string>

#include "lib/data_sources/live_service.hpp"
#include "lib/drivers/leds/led_handler.hpp"
#include "lib/system/shared.hpp"
#include "lib/system/timers.hpp"
#include "lib/ui/screen_handler.hpp"

class BlockNotify : public LiveService {
public:
  static BlockNotify &getInstance() {
    static BlockNotify instance;
    return instance;
  }

  // Delete copy constructor and assignment operator
  BlockNotify(const BlockNotify &) = delete;
  void operator=(const BlockNotify &) = delete;

  // Block notification setup and control
  void setup();
  void stop();
  void restart() override;
  bool isConnected() const override;
  bool isInitialized() const override;

  // LiveService identity + watchdog policy
  const char *name() const override { return "BlockNotify"; }
  unsigned long lastUpdateSeconds() const override {
    return lastBlockUpdate.load(std::memory_order_relaxed);
  }
  // No new block in 45 minutes means something is wrong upstream. Aligns
  // with the ad-hoc "checkMissedBlocks" threshold from main.cpp.
  unsigned long staleAfterSeconds() const override { return 45UL * 60UL; }

  // Block height management
  void setBlockHeight(uint32_t newBlockHeight);
  uint32_t getBlockHeight() const;

  // Block fee management
  void setBlockMedianFee(float blockMedianFee);
  float getBlockMedianFee() const;

  // Block processing
  void processNewBlock(uint32_t newBlockHeight);
  void processNewBlockFee(float newBlockFee);

  // Block fetch and update tracking
  int fetchLatestBlock();
  uint getLastBlockUpdate() const;
  void setLastBlockUpdate(uint lastUpdate);

private:
  BlockNotify() = default; // Private constructor for singleton

  static void onWebsocketEvent(WStype_t type, uint8_t *payload, size_t length);
  void onWebsocketMessage(uint8_t *payload, size_t length);
  static void taskBlockNotify(void *pvParameters);

  static WebSocketsClient wsClient;
  // These are written from the mempool.space WebSocket callback (block
  // notify pump task) and read from the webserver/SSE/display tasks.
  // std::atomic gives us a defined memory model for the cross-task reads
  // without adding a mutex per getter.
  static std::atomic<uint32_t> currentBlockHeight;
  static std::atomic<float> blockMedianFee;
  static std::atomic<bool> notifyInit;
  static std::atomic<bool> wsConnected;
  static std::atomic<unsigned long> lastBlockUpdate;
  // Set by stop() to ask the pump task to exit cleanly rather than being
  // torn down mid-wsClient.loop() via vTaskDelete, which can leave the
  // WebSocket client's internal state inconsistent for subsequent
  // beginSSL() calls. The task clears this flag and deletes itself.
  static std::atomic<bool> shouldStop;
  static TaskHandle_t taskHandle;
};