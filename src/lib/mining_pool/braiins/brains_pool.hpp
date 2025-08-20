#pragma once

#include "lib/mining_pool/mining_pool_interface.hpp"
#include <icons/icons.h>
#include <utils.hpp>

class BraiinsPool : public MiningPoolInterface
{
public:
    void setPoolUser(const std::string &user) override { poolUser = user; }
    void prepareRequest(HTTPClient &http) const override;
    std::string getApiUrl() const override;
    PoolStats parseResponse(const JsonDocument &doc) const override;
    bool supportsDailyEarnings() const override { return true; }
    bool hasLogo() const override { return true; }
    std::string getDisplayLabel() const override { return "BRAIINS/POOL"; } // Fallback if needed
    std::string getDailyEarningsLabel() const override { return "sats/earned"; }
    std::string getLogoFilename() const override {
        return "braiins.bin";
    }

    std::string getPoolName() const override {
        return "braiins";
    }

    int getLogoWidth() const override {
        return 37;
    }

    int getLogoHeight() const override {
        return 230;
    }
};