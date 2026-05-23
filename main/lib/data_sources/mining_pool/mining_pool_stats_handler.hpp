#include <array>
#include <iostream>
#include <string>
#include <utils.hpp>

#ifndef UNITY_TEST
#include "lib/data_sources/mining_pool/mining_pool_interface.hpp"
#endif

std::array<std::string, NUM_SCREENS>
parseMiningPoolStatsHashRate(const std::string &hashrate,
                             const MiningPoolInterface &pool);
std::array<std::string, NUM_SCREENS>
parseMiningPoolStatsDailyEarnings(int64_t sats, std::string label,
                                  const MiningPoolInterface &pool);
