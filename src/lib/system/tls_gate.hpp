#pragma once

#include <mutex>

// Single lock that serialises every TLS handshake across the firmware.
//
// Each in-flight mbedtls SSL context holds ~16 KB of IN buffer plus
// scratch space; on Rev B the DRAM pool is ~350 KB shared across Wi-Fi,
// LWIP, display, data-source WS clients, HTTPS pollers and the WebUI
// async server. With THIRD_PARTY_SOURCE (mempool WS + Kraken WS) plus
// Nostr WS plus bitaxe HTTPS plus mining-pool HTTPS all starting up at
// boot, the moment when three or four handshakes overlap is the moment
// heap briefly dips below what a fresh mbedtls session needs. The
// second handshake then fails to alloc, surfaces as
// `Error retrieving X. HTTP status code: -1` or a WS stuck at
// "Connection Closed".
//
// Taking this mutex around the narrow window in which a TLS handshake
// is actually happening — i.e. HTTPClient::GET() for HTTPS pollers, and
// `ws.loop()` when the WebSocketsClient is NOT already connected —
// forces those handshakes to happen sequentially. Peak heap pressure
// drops from "four contexts alive at once" to "one", while the steady-
// state fast path (`ws.isConnected()` → call loop without lock) stays
// contention-free.
namespace tls_gate {

inline std::mutex &mutex() {
  static std::mutex m;
  return m;
}

} // namespace tls_gate
