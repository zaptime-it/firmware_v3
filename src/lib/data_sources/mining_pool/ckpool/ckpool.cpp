#include "ckpool.hpp"

#include <cstdlib>

void CKPool::prepareRequest(HTTPClient &http) const {
  // Empty as CKPool doesn't need special headers
}

std::string CKPool::getApiUrl() const {
  return getBaseUrl() + "/users/" + poolUser;
}

PoolStats CKPool::parseResponse(const JsonDocument &doc) const {
  const PoolStats fallback{"0", std::nullopt};

  std::string hashrateStr = doc["hashrate1m"].as<std::string>();
  if (hashrateStr.empty() || hashrateStr == "0") {
    return fallback;
  }

  char unit = hashrateStr.back();
  std::string value = hashrateStr.substr(0, hashrateStr.size() - 1);

  char *endp = nullptr;
  double parsed = std::strtod(value.c_str(), &endp);
  if (endp == value.c_str()) {
    return fallback;
  }

  int multiplier = getHashrateMultiplier(unit);
  double hashrate = parsed * std::pow(10, multiplier);

  char buffer[32];
  snprintf(buffer, sizeof(buffer), "%.0f", hashrate);

  return PoolStats{.hashrate = buffer, .dailyEarnings = std::nullopt};
}
