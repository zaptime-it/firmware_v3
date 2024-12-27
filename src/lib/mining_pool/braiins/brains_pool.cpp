#include "brains_pool.hpp"

void BraiinsPool::prepareRequest(HTTPClient& http) const {
    http.addHeader("Pool-Auth-Token", poolUser.c_str());
}

std::string BraiinsPool::getApiUrl() const {
    return "https://pool.braiins.com/accounts/profile/json/btc/";
}

PoolStats BraiinsPool::parseResponse(const JsonDocument &doc) const
{
    if (doc["btc"].isNull()) {
        return PoolStats{
            .hashrate = "0",
            .dailyEarnings = 0
        };
    }

    std::string unit = doc["btc"]["hash_rate_unit"].as<std::string>();

    static const std::unordered_map<std::string, int> multipliers = {
        {"Zh/s", 21}, {"Eh/s", 18}, {"Ph/s", 15}, {"Th/s", 12}, {"Gh/s", 9}, {"Mh/s", 6}, {"Kh/s", 3}};

    int multiplier = multipliers.at(unit);
    float hashValue = doc["btc"]["hash_rate_5m"].as<float>();

    return PoolStats{
        .hashrate = std::to_string(static_cast<int>(std::round(hashValue))) + std::string(multiplier, '0'),
        .dailyEarnings = static_cast<int64_t>(doc["btc"]["today_reward"].as<float>() * 100000000)};
}

