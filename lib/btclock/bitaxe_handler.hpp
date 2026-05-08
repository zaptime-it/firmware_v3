#include "utils.hpp"
#include <array>
#include <cstdint>
#include <string>

std::array<std::string, NUM_SCREENS> parseBitaxeHashRate(uint64_t hashrate);
std::array<std::string, NUM_SCREENS> parseBitaxeBestDiff(uint64_t difficulty);
