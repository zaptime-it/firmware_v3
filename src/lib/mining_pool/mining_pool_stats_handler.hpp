#include <array>
#include <string>
#ifndef UNITY_TEST
#include "lib/mining_pool/mining_pool_interface.hpp"
#endif

std::array<std::string, NUM_SCREENS> parseMiningPoolStatsHashRate(std::string text, const MiningPoolInterface& pool);
std::array<std::string, NUM_SCREENS> parseMiningPoolStatsDailyEarnings(int sats, std::string label, const MiningPoolInterface& pool);
