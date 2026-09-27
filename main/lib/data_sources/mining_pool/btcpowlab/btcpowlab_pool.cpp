#include "btcpowlab_pool.hpp"
#include <btc_pow_lab_stats.hpp>

std::string BtcPowLabPool::getApiUrl() const {
  return "https://btcpowlab-pool.com/public/v1/miner/" + poolUser + "/summary";
}

PoolStats BtcPowLabPool::parseResponse(const JsonDocument &doc) const {
  return PoolStats{.hashrate = btclock::selectBtcPowLabHashrate(
                       doc["hashrate_1h_hs"].as<double>(),
                       doc["hashrate_5m_hs"].as<double>()),
                   .dailyEarnings = std::nullopt};
}
