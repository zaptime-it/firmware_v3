#include "live_service.hpp"

#include <esp_timer.h>

namespace {

// Window after a disconnect before we call restart() on the service.
// Matches the prior bespoke 5-minute threshold in
// handlePriceNotifyDisconnection and handleBlockNotifyDisconnection.
constexpr unsigned long kDisconnectRestartSeconds = 300;

unsigned long uptimeSeconds() {
  return static_cast<unsigned long>(esp_timer_get_time() / 1000000);
}

} // namespace

LiveServiceRegistry &LiveServiceRegistry::instance() {
  static LiveServiceRegistry r;
  return r;
}

void LiveServiceRegistry::registerService(LiveService *service) {
  if (service == nullptr)
    return;
  for (const auto &e : entries) {
    if (e.service == service)
      return;
  }
  entries.push_back({service, 0});
}

void LiveServiceRegistry::unregisterService(LiveService *service) {
  for (auto it = entries.begin(); it != entries.end(); ++it) {
    if (it->service == service) {
      entries.erase(it);
      return;
    }
  }
}

void LiveServiceRegistry::clear() { entries.clear(); }

void LiveServiceRegistry::monitor() {
  const unsigned long now = uptimeSeconds();

  for (auto &entry : entries) {
    LiveService *service = entry.service;
    if (service == nullptr || !service->isInitialized()) {
      entry.disconnectedSince = 0;
      continue;
    }

    if (!service->isConnected()) {
      if (entry.disconnectedSince == 0) {
        entry.disconnectedSince = now;
        Serial.printf(
            "[ LiveService ] %s disconnected, watching for reconnect...\r\n",
            service->name());
      } else if ((now - entry.disconnectedSince) > kDisconnectRestartSeconds) {
        Serial.printf("[ LiveService ] %s still disconnected after %lus, "
                      "restarting...\r\n",
                      service->name(), kDisconnectRestartSeconds);
        service->restart();
        entry.disconnectedSince = 0;
      }
      continue;
    }

    // Connected again: clear the disconnect timer.
    entry.disconnectedSince = 0;

    // Staleness watchdog: restart a service whose connection is up but
    // which has not delivered data within its expected window (e.g.
    // price ticks, new blocks).
    const unsigned long staleWindow = service->staleAfterSeconds();
    const unsigned long lastUpdate = service->lastUpdateSeconds();
    if (staleWindow > 0 && lastUpdate != 0 && now > lastUpdate &&
        (now - lastUpdate) > staleWindow) {
      Serial.printf(
          "[ LiveService ] %s stale for %lus (> %lus), restarting...\r\n",
          service->name(), now - lastUpdate, staleWindow);
      service->restart();
    }
  }
}
