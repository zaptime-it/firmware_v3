
#pragma once

#include "lib/mining_pool/mining_pool_interface.hpp"
#include <icons/icons.h>

class NodeRunnersPool : public MiningPoolInterface {
public:
    void setPoolUser(const std::string& user) override { poolUser = user; }

    void prepareRequest(HTTPClient& http) const override;
    std::string getApiUrl() const override;
    PoolStats parseResponse(const JsonDocument& doc) const override;
    LogoData getLogo() const override;
    bool supportsDailyEarnings() const override { return false; }
    std::string getDailyEarningsLabel() const override { return ""; }

private:
    static int getHashrateMultiplier(char unit);
};