#include <array>
#include <string>
#include <iostream>
#include <utils.hpp>

#ifndef UNITY_TEST
#include "lib/mining_pool/mining_pool_interface.hpp"
#endif

std::array<std::string, NUM_SCREENS> parseMiningPoolStatsHashRate(const std::string& hashrate, const MiningPoolInterface& pool);
std::array<std::string, NUM_SCREENS> parseMiningPoolStatsDailyEarnings(int sats, std::string label, const MiningPoolInterface& pool);
