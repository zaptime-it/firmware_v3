#include <array>
#include <string>

std::array<std::string, NUM_SCREENS> parseMiningPoolStatsHashRate(std::string text);
std::array<std::string, NUM_SCREENS> parseMiningPoolStatsDailyEarnings(int sats, std::string label);
