// src/noderunners/noderunners_pool.cpp
#include "public_pool.hpp"

std::string PublicPool::getApiUrl() const
{
    return "https://public-pool.io:40557/api/client/" + poolUser;
}

PoolStats PublicPool::parseResponse(const JsonDocument &doc) const
{
    uint64_t totalHashrate = 0;

    try
    {
        for (JsonVariantConst worker : doc["workers"].as<JsonArrayConst>())
        {
            totalHashrate += static_cast<uint64_t>(std::llround(worker["hashRate"].as<double>()));
        }
    }
    catch (const std::exception &e)
    {
        Serial.printf("Error parsing %s response: %s\n", getPoolName().c_str(), e.what());
        return PoolStats{
            .hashrate = "0",
            .dailyEarnings = std::nullopt};
    }

    return PoolStats{
        .hashrate = std::to_string(totalHashrate),
        .dailyEarnings = std::nullopt // Public Pool doesn't support daily earnings
    };
}