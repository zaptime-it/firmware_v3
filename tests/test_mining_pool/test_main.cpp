#include <unity.h>
#include <utils.hpp>

void test_parseMiningPoolStatsHashRate1dot34TH(void) {
  std::string label;
  std::string output;

  parseHashrateString("1340000000000", label, output, 4);

  TEST_ASSERT_EQUAL_STRING_MESSAGE("TH/S", label.c_str(), label.c_str());
  TEST_ASSERT_EQUAL_STRING_MESSAGE("1.34", output.c_str(), output.c_str());
}

void test_parseMiningPoolStatsHashRate645GH(void) {
  std::string hashrate = "645000000000";
  std::string label;
  std::string output;

  parseHashrateString(hashrate, label, output, 4);

  TEST_ASSERT_EQUAL_STRING_MESSAGE("GH/S", label.c_str(), label.c_str());
  TEST_ASSERT_EQUAL_STRING_MESSAGE("645", output.c_str(), output.c_str());
}

void test_parseMiningPoolStatsHashRateEmpty(void) {
  std::string hashrate;
  std::string label;
  std::string output;

  parseHashrateString(hashrate, label, output, 4);

  TEST_ASSERT_EQUAL_STRING_MESSAGE("H/S", label.c_str(), label.c_str());
  TEST_ASSERT_EQUAL_STRING_MESSAGE("0", output.c_str(), output.c_str());
}

void test_parseMiningPoolStatsHashRateZero(void) {
  std::string hashrate = "0";
  std::string label;
  std::string output;

  parseHashrateString(hashrate, label, output, 4);

  TEST_ASSERT_EQUAL_STRING_MESSAGE("H/S", label.c_str(), label.c_str());
  TEST_ASSERT_EQUAL_STRING_MESSAGE("0", output.c_str(), output.c_str());
}

// ---------------------------------------------------------------------------
// Extra parseHashrateString regressions that exercise very large values and
// the digit-count argument used by the mining-pool screens.
// ---------------------------------------------------------------------------

void test_parseHashrateString_ZettaHash(void) {
  std::string label;
  std::string output;
  // 1.0 ZH/s (1e21) — previously the table lookup used .at() which threw
  // std::out_of_range. With the .find() fallback we simply hit the largest
  // known multiplier and label as ZH/S.
  parseHashrateString("1000000000000000000000", label, output, 4);
  TEST_ASSERT_EQUAL_STRING("ZH/S", label.c_str());
  TEST_ASSERT_EQUAL_STRING("1", output.c_str());
}

void test_parseHashrateString_Garbage(void) {
  std::string label;
  std::string output;
  // std::stod previously threw on this input, now we should get a safe
  // "0 H/S" result instead of crashing.
  parseHashrateString("not-a-number", label, output, 4);
  TEST_ASSERT_EQUAL_STRING("H/S", label.c_str());
  TEST_ASSERT_EQUAL_STRING("0", output.c_str());
}

void test_parseHashrateString_MaxCharsWideInteger(void) {
  std::string label;
  std::string output;
  // 123.456 GH/s — maxCharacters is the target precision budget.  When the
  // integer part already exceeds it we fall back to rounding to int rather
  // than producing negative printf precisions.
  parseHashrateString("123456789012", label, output, 2);
  TEST_ASSERT_EQUAL_STRING("GH/S", label.c_str());
  TEST_ASSERT_EQUAL_STRING("123", output.c_str());
}

// not needed when using generate_test_runner.rb
int runUnityTests(void) {
  UNITY_BEGIN();
  RUN_TEST(test_parseMiningPoolStatsHashRate1dot34TH);
  RUN_TEST(test_parseMiningPoolStatsHashRate645GH);
  RUN_TEST(test_parseMiningPoolStatsHashRateZero);
  RUN_TEST(test_parseMiningPoolStatsHashRateEmpty);
  RUN_TEST(test_parseHashrateString_ZettaHash);
  RUN_TEST(test_parseHashrateString_Garbage);
  RUN_TEST(test_parseHashrateString_MaxCharsWideInteger);

  return UNITY_END();
}

int main(void) { return runUnityTests(); }

extern "C" void app_main() { runUnityTests(); }
