#pragma once

#include "lib/mining_pool/mining_pool_interface.hpp"
#include <icons/icons.h>
#include <utils.hpp>

class NoderunnersPool : public MiningPoolInterface {
public:
    void setPoolUser(const std::string& user) override { poolUser = user; }

    void prepareRequest(HTTPClient& http) const override;
    std::string getApiUrl() const override;
    PoolStats parseResponse(const JsonDocument& doc) const override;
    bool supportsDailyEarnings() const override { return false; }
    std::string getDailyEarningsLabel() const override { return ""; }
    bool hasLogo() const override { return true; }
    std::string getDisplayLabel() const override { return "NODE/RUNNERS"; } // Fallback if needed
    std::string getLogoFilename() const override {
        return "noderunners.bin";
    }

    std::string getPoolName() const override {
        return "noderunners";
    }

    int getLogoWidth() const override {
        return 122;
    }

    int getLogoHeight() const override {
        return 122;
    }
};