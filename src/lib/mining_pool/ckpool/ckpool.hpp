#pragma once

#include "lib/mining_pool/mining_pool_interface.hpp"
#include <utils.hpp>

class CKPool : public MiningPoolInterface {
public:
    void setPoolUser(const std::string& user) override { poolUser = user; }

    void prepareRequest(HTTPClient& http) const override;
    std::string getApiUrl() const override;
    PoolStats parseResponse(const JsonDocument& doc) const override;
    bool supportsDailyEarnings() const override { return false; }
    std::string getDailyEarningsLabel() const override { return ""; }
    bool hasLogo() const override { return false; }
    std::string getDisplayLabel() const override { return "CK/POOL"; }
    std::string getPoolName() const override {
        return "ckpool";
    }

protected:
    virtual std::string getBaseUrl() const {
        return "https://solo.ckpool.org";
    }
}; 