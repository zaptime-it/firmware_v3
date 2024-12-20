// src/noderunners/noderunners_pool.cpp
#include "noderunners_pool.hpp"

void NoderunnersPool::prepareRequest(HTTPClient& http) const {
    // Empty as NodeRunners doesn't need special headers
}

std::string NoderunnersPool::getApiUrl() const {
    return "https://pool.noderunners.network/api/v1/users/" + poolUser;
}

PoolStats NoderunnersPool::parseResponse(const JsonDocument& doc) const {
    std::string hashrateStr = doc["hashrate1m"].as<std::string>();
    char unit = hashrateStr.back();
    std::string value = hashrateStr.substr(0, hashrateStr.size() - 1);
    
    int multiplier = getHashrateMultiplier(unit);
    double hashrate = std::stod(value) * std::pow(10, multiplier);
    
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%.0f", hashrate);
    
    return PoolStats{
        .hashrate = buffer,
        .dailyEarnings = std::nullopt
    };
}

LogoData NoderunnersPool::getLogo() const {
    return LogoData {
        .data = epd_icons_allArray[6],
        .width = 122,
        .height = 122
    };
}