// src/noderunners/noderunners_pool.cpp
#include "noderunners_pool.hpp"

void NodeRunnersPool::prepareRequest(HTTPClient& http) const {
    // Empty as NodeRunners doesn't need special headers
}

std::string NodeRunnersPool::getApiUrl() const {
    return "https://pool.noderunners.network/api/v1/users/" + poolUser;
}

PoolStats NodeRunnersPool::parseResponse(const JsonDocument& doc) const {
    std::string hashrateStr = doc["hashrate1m"].as<std::string>();
    char unit = hashrateStr.back();
    std::string value = hashrateStr.substr(0, hashrateStr.size() - 1);
    
    int multiplier = getHashrateMultiplier(unit);
    
    return PoolStats{
        .hashrate = value + std::string(multiplier, '0'),
        .dailyEarnings = std::nullopt
    };
}

LogoData NodeRunnersPool::getLogo() const {
    return LogoData {
        .data = epd_icons_allArray[6],
        .width = 122,
        .height = 122
    };
}

int NodeRunnersPool::getHashrateMultiplier(char unit) {
    static const std::unordered_map<char, int> multipliers = {
        {'Z', 21}, {'E', 18}, {'P', 15}, {'T', 12},
        {'G', 9},  {'M', 6},  {'K', 3}
    };
    return multipliers.at(unit);
}
