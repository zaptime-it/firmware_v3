#include "btc_pow_lab_stats.hpp"

#include <cmath>
#include <cstdio>

namespace btclock {

std::string selectBtcPowLabHashrate(double hashrate1h, double hashrate5m) {
  double selected = hashrate1h;
  if (!std::isfinite(selected) || selected <= 0) {
    selected = hashrate5m;
  }

  if (!std::isfinite(selected) || selected <= 0) {
    return "0";
  }

  char buffer[32];
  snprintf(buffer, sizeof(buffer), "%.0f", selected);
  return buffer;
}

} // namespace btclock
