#include "mining_pool_stats_handler.hpp"

std::array<std::string, NUM_SCREENS>
parseMiningPoolStatsHashRate(const std::string &hashrate,
                             const MiningPoolInterface &pool) {
  std::array<std::string, NUM_SCREENS> ret;
  ret.fill(""); // Initialize all elements to empty strings
  std::string label;
  std::string output;

  parseHashrateString(hashrate, label, output, 4);

  std::size_t textLength = output.length();
  // Defensive clamp: reserve slot 0 (logo/label) and slot NUM_SCREENS-1
  // (units) so startIndex (size_t) never underflows if parseHashrateString
  // ever returns a string longer than the display can hold.
  const std::size_t maxDigits = NUM_SCREENS - 2;
  if (textLength > maxDigits) {
    output = output.substr(0, maxDigits);
    textLength = output.length();
  }
  std::size_t startIndex = NUM_SCREENS - 1 - textLength;

  // Insert the pickaxe icon just before the digits, but never at [0]
  // which is reserved for the pool logo/label.
  if (startIndex > 1) {
    ret[startIndex - 1] = "mdi:pickaxe";
  }

  // Place the digits
  for (std::size_t i = 0; i < textLength; ++i) {
    ret[startIndex + i] = output.substr(i, 1);
  }

  ret[NUM_SCREENS - 1] = label;

  if (pool.hasLogo()) {
    ret[0] = "mdi:miningpool";
  } else {
    ret[0] = pool.getDisplayLabel();
  }

  return ret;
}

std::array<std::string, NUM_SCREENS>
parseMiningPoolStatsDailyEarnings(int64_t sats, std::string label,
                                  const MiningPoolInterface &pool) {
  std::array<std::string, NUM_SCREENS> ret;
  ret.fill(""); // Initialize all elements to empty strings
  std::string satsDisplay = std::to_string(sats);

  if (sats >= 100000000LL) {
    // A whale mining 1+ BTC per day! No decimal points; whales scoff at such
    // things.
    label = "BTC" + label.substr(4);
    satsDisplay = satsDisplay.substr(0, satsDisplay.length() - 8);
  } else if (sats >= 10000000) {
    // 10.0M to 99.9M you get one decimal point
    satsDisplay = satsDisplay.substr(0, satsDisplay.length() - 6) + "." +
                  satsDisplay[2] + "M";
  } else if (sats >= 1000000) {
    // 1.00M to 9.99M you get two decimal points
    satsDisplay = satsDisplay.substr(0, satsDisplay.length() - 6) + "." +
                  satsDisplay.substr(2, 2) + "M";
  } else if (sats >= 100000) {
    // 100K to 999K you get no extra precision
    satsDisplay = satsDisplay.substr(0, satsDisplay.length() - 3) + "K";
  } else if (sats >= 10000) {
    // 10.0K to 99.9K you get one decimal point
    satsDisplay = satsDisplay.substr(0, satsDisplay.length() - 3) + "." +
                  satsDisplay[2] + "K";
  } else {
    // Pleb miner! 4 digit or fewer sats will fit as-is. no-op.
  }

  std::size_t textLength = satsDisplay.length();

  // Defensive clamp to prevent size_t underflow.
  const std::size_t maxEarningDigits = NUM_SCREENS - 2;
  if (textLength > maxEarningDigits) {
    satsDisplay = satsDisplay.substr(0, maxEarningDigits);
    textLength = satsDisplay.length();
  }
  std::size_t startIndex = NUM_SCREENS - 1 - textLength;

  // Insert the pickaxe icon just before the digits, but never at [0].
  if (startIndex > 1) {
    ret[startIndex - 1] = "mdi:pickaxe";
  }

  // Place the digits
  for (std::size_t i = 0; i < textLength; ++i) {
    ret[startIndex + i] = satsDisplay.substr(i, 1);
  }

  ret[NUM_SCREENS - 1] = label;

  if (pool.hasLogo()) {
    ret[0] = "mdi:miningpool";
  } else {
    ret[0] = pool.getDisplayLabel();
  }

  return ret;
}