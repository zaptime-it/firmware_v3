// src/noderunners/noderunners_pool.cpp
#include "noderunners_pool.hpp"

void NoderunnersPool::prepareRequest(HTTPClient &http) const
{
    // Empty as Noderunners doesn't need special headers
}

std::string NoderunnersPool::getApiUrl() const
{
    return "https://pool.noderunners.network/api/v1/users/" + poolUser;
}

PoolStats NoderunnersPool::parseResponse(const JsonDocument &doc) const
{
    try
    {
        std::string hashrateStr = doc["hashrate1m"].as<std::string>();
        
        // Special case for "0"
        if (hashrateStr == "0") {
            return PoolStats{
                .hashrate = "0",
                .dailyEarnings = std::nullopt
            };
        }

        char unit = hashrateStr.back();
        std::string value = hashrateStr.substr(0, hashrateStr.size() - 1);

        int multiplier = getHashrateMultiplier(unit);
        double hashrate = std::stod(value) * std::pow(10, multiplier);

        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%.0f", hashrate);

        return PoolStats{
            .hashrate = buffer,
            .dailyEarnings = std::nullopt};
    }
    catch (const std::exception &e)
    {
        Serial.printf("Error parsing %s response: %s\n", getPoolName().c_str(), e.what());
        return PoolStats{
            .hashrate = "0",
            .dailyEarnings = std::nullopt};
    }
}