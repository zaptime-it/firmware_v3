#pragma once

namespace data_source_policy {

// Gate for the "data source disconnected" LED effect.
//
// Historically every WebSocket disconnect after the first would queue a
// purple LED_DATA_BLOCK_ERROR blink. When the root cause is WiFi being
// down, the dedicated WiFi LED effect is already firing, and the socket
// auto-reconnects every 5 s — so the device ends up strobing a long
// purple-red sequence until WiFi returns, which adds no information.
//
// With this gate the data-source effect only fires when WiFi is actually
// up (meaning the server-side or network-path is the real culprit).
inline bool shouldFlashDataSourceError(int disconnectCount,
                                       bool wifiConnected) {
  return disconnectCount > 1 && wifiConnected;
}

} // namespace data_source_policy
