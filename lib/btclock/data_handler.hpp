#include <array>
#include <string>
#include <cmath>
#include <cstdint>
#include <vector>

#include "utils.hpp"

const char CURRENCY_USD = '$';
const char CURRENCY_EUR = '[';
const char CURRENCY_GBP = ']';
const char CURRENCY_JPY = '^';
const char CURRENCY_AUD = '_';
const char CURRENCY_CAD = '`';

const std::string CURRENCY_CODE_USD = "USD";
const std::string CURRENCY_CODE_EUR = "EUR";
const std::string CURRENCY_CODE_GBP = "GBP";
const std::string CURRENCY_CODE_JPY = "JPY";
const std::string CURRENCY_CODE_AUD = "AUD";
const std::string CURRENCY_CODE_CAD = "CAD";

std::array<std::string, NUM_SCREENS> parsePriceData(std::uint32_t price, char currency, bool useSuffixFormat = false, bool mowMode = false, bool shareDot = false);
std::array<std::string, NUM_SCREENS> parseSatsPerCurrency(std::uint32_t price, char currencySymbol, bool withSatsSymbol);
std::array<std::string, NUM_SCREENS> parseBlockHeight(std::uint32_t blockHeight);
std::array<std::string, NUM_SCREENS> parseHalvingCountdown(std::uint32_t blockHeight, bool asBlocks);
std::array<std::string, NUM_SCREENS> parseMarketCap(std::uint32_t blockHeight, std::uint32_t price, char currencySymbol, bool bigChars);
std::array<std::string, NUM_SCREENS> parseBlockFees(float blockFees);

char getCurrencySymbol(char input);
std::string getCurrencyCode(char input);
char getCurrencyChar(const std::string& input);