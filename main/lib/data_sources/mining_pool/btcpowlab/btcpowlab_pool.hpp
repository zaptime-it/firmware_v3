#pragma once

#include "lib/data_sources/mining_pool/noderunners/noderunners_pool.hpp"

class BtcPowLabPool : public NoderunnersPool {
public:
  std::string getApiUrl() const override;
  PoolStats parseResponse(const JsonDocument &doc) const override;
  bool supportsGlobalStats() const override { return false; }
  bool hasLogo() const override { return false; }
  std::string getDisplayLabel() const override { return "BTC/POW/LAB"; }
  std::string getPoolName() const override { return "btcpowlab"; }
};
