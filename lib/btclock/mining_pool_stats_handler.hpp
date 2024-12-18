#include <array>
#include <string>

std::array<std::string, NUM_SCREENS> parseMiningPoolStatsHashRate(std::string miningPoolName, std::string text);
std::array<std::string, NUM_SCREENS> parseMiningPoolStatsDailyEarnings(std::string miningPoolName, int sats);
