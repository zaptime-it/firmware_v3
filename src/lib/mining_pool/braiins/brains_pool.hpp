#pragma once

#include "lib/mining_pool/mining_pool_interface.hpp"
#include <icons/icons.h>

class BraiinsPool : public MiningPoolInterface
{
public:
    void setPoolUser(const std::string &user) override { poolUser = user; }
    void prepareRequest(HTTPClient &http) const override;
    std::string getApiUrl() const override;
    PoolStats parseResponse(const JsonDocument &doc) const override;
    LogoData getLogo() const override;
    bool supportsDailyEarnings() const override { return true; }
    bool hasLogo() const override { return true; }
    std::string getDisplayLabel() const override { return "BRAIINS/POOL"; } // Fallback if needed
    std::string getDailyEarningsLabel() const override { return "sats/earned"; }
private:
    static int getHashrateMultiplier(const std::string &unit);
};