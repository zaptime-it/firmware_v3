#pragma once

#include <string>
#include <cmath>
#include <cstdint>
#include <sstream>
#include <iomanip>
#include <unordered_map>


int modulo(int x,int N);

double getSupplyAtBlock(std::uint32_t blockNr);

std::string formatNumberWithSuffix(std::uint64_t num, int numCharacters = 4);
std::string formatNumberWithSuffix(std::uint64_t num, int numCharacters, bool mowMode);

int64_t getAmountInSatoshis(std::string bolt11);
void parseHashrateString(const std::string& hashrate, std::string& label, std::string& output, unsigned int maxCharacters);
int getHashrateMultiplier(char unit);
int getDifficultyMultiplier(char unit);