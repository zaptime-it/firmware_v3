#include "ocean_pool.hpp"

void OceanPool::prepareRequest(HTTPClient &http) const {
  // Empty as Ocean doesn't need special headers
}

std::string OceanPool::getApiUrl() const {
  return "https://api.ocean.xyz/v1/statsnap/" + poolUser;
}

PoolStats OceanPool::parseResponse(const JsonDocument &doc) const {
  if (doc["result"].isNull()) {
    return PoolStats{.hashrate = "0", .dailyEarnings = std::nullopt};
  }

  return PoolStats{
      .hashrate = doc["result"]["hashrate_300s"].as<std::string>(),
      .dailyEarnings = static_cast<int64_t>(
          doc["result"]["estimated_earn_next_block"].as<float>() * 100000000)};
}