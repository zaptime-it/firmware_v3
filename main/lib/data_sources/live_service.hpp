#pragma once

#include <Arduino.h>
#include <vector>

// Common interface for every long-lived "live data" service (block notify,
// price notify, v2 notify, nostr notify, bitaxe, mining pool stats...).
// Implementing this and registering the instance with LiveServiceRegistry
// lets monitorDataConnections() work generically instead of hard-coding
// BlockNotify + PriceNotify. That's what finally closes the watchdog gap
// for BTCLOCK_SOURCE / CUSTOM_SOURCE / NOSTR_SOURCE modes.
class LiveService {
public:
  virtual ~LiveService() = default;

  // Short stable identifier used in log messages.
  virtual const char *name() const = 0;

  // True when the underlying connection/HTTP client has completed its
  // first successful setup. Used to avoid warning about "disconnected"
  // services that never had a chance to connect in the first place.
  virtual bool isInitialized() const = 0;

  // True when the upstream is currently reachable. For HTTP-poll style
  // services (bitaxe, pool stats) this can just track the last poll
  // status; they can opt out of the disconnect watchdog entirely by
  // returning true here.
  virtual bool isConnected() const = 0;

  // Epoch seconds (via esp_timer-derived uptime) of the last successful
  // data event. Zero means "never". Used for the staleness watchdog.
  virtual unsigned long lastUpdateSeconds() const = 0;

  // How long a data service may stay stale before we force a restart()
  // even if isConnected() says it is up. Zero disables the staleness
  // watchdog for this service.
  virtual unsigned long staleAfterSeconds() const { return 0; }

  // Tear down and rebuild the underlying connection. Must be safe to
  // call repeatedly.
  virtual void restart() = 0;
};

// Process-wide registry of active LiveService instances. Services opt in
// via registerService() during setupDataSource() and opt out via
// unregisterService() when the data source is switched off. Iteration is
// only from the main loop task, so this type is intentionally not
// thread-safe.
class LiveServiceRegistry {
public:
  static LiveServiceRegistry &instance();

  void registerService(LiveService *service);
  void unregisterService(LiveService *service);
  void clear();

  // Called every ~5s from the main loop. Iterates registered services,
  // tracks first-seen-disconnect-at, and calls restart() after a
  // disconnect or staleness window expires.
  void monitor();

private:
  struct Entry {
    LiveService *service;
    unsigned long disconnectedSince; // 0 == "currently connected"
  };
  std::vector<Entry> entries;
};
