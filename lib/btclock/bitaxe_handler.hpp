#include <array>
#include <string>
#include <cstdint>
#include "utils.hpp"

std::array<std::string, NUM_SCREENS> parseBitaxeHashRate(uint64_t hashrate);
std::array<std::string, NUM_SCREENS> parseBitaxeBestDiff(uint64_t difficulty);
