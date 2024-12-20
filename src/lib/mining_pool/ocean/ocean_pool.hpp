#pragma once

#include "lib/mining_pool/mining_pool_interface.hpp"
#include <icons/icons.h>

class OceanPool : public MiningPoolInterface {
public:
    void setPoolUser(const std::string& user) override { poolUser = user; }
    void prepareRequest(HTTPClient& http) const override;
    std::string getApiUrl() const override;
    PoolStats parseResponse(const JsonDocument& doc) const override;
    bool hasLogo() const override { return true; }
    std::string getDisplayLabel() const override { return "OCEAN/POOL"; } // Fallback if needed
    bool supportsDailyEarnings() const override { return true; }
    std::string getDailyEarningsLabel() const override { return "sats/block"; }
    std::string getLogoFilename() const override {
        return "ocean.bin";
    }

    std::string getPoolName() const override {
        return "ocean";
    }

    int getLogoWidth() const override {
        return 122;
    }

    int getLogoHeight() const override {
        return 122;
    }
};