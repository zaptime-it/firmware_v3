#pragma once

#include "lib/system/shared.hpp"

#include <ArduinoJson.h>
#include <nostrdisplay_handler.hpp>
#include <string>

#include "NostrEvent.h"
#include "NostrPool.h"
#include "esp32/ESP32Platform.h"

#include "block_notify.hpp"
#include "lib/data_sources/live_service.hpp"
#include "lib/system/timers.hpp"
#include "price_notify.hpp"

void setupNostrNotify(bool asDatasource, bool zapNotify);
void stopNostrNotify();
void restartNostrNotify();

void nostrTask(void *pvParameters);
void setupNostrTask();

boolean nostrConnected();
bool nostrIsInitialized();
unsigned long getLastNostrUpdate();

// Adapter exposing NostrNotify (as a data source) through LiveService.
class NostrNotifyService : public LiveService {
public:
  static NostrNotifyService &getInstance() {
    static NostrNotifyService instance;
    return instance;
  }
  const char *name() const override { return "NostrNotify"; }
  bool isInitialized() const override { return nostrIsInitialized(); }
  bool isConnected() const override { return nostrConnected(); }
  unsigned long lastUpdateSeconds() const override {
    return getLastNostrUpdate();
  }
  // Nostr relays can be quiet for a while; stay tolerant. 15 minute window.
  unsigned long staleAfterSeconds() const override { return 15UL * 60UL; }
  void restart() override { restartNostrNotify(); }
};
void handleNostrEventCallback(const String &subId,
                              nostr::SignedNostrEvent *event);
void handleNostrZapCallback(const String &subId,
                            nostr::SignedNostrEvent *event);

void onNostrSubscriptionClosed(const String &subId, const String &reason);
void onNostrSubscriptionEose(const String &subId);

time_t getMinutesAgo(int min);
void subscribeZaps(nostr::NostrPool *pool, const String &relay, int minutesAgo);