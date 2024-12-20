#pragma once

#include "lib/mining_pool/mining_pool_interface.hpp"
#include <icons/icons.h>

class OceanPool : public MiningPoolInterface {
public:
    void setPoolUser(const std::string& user) override { poolUser = user; }
    void prepareRequest(HTTPClient& http) const override;
    std::string getApiUrl() const override;
    PoolStats parseResponse(const JsonDocument& doc) const override;
    LogoData getLogo() const override;
    bool supportsDailyEarnings() const override { return true; }
    std::string getDailyEarningsLabel() const override { return "sats/block"; }
};