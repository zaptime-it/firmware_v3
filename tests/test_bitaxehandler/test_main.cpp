#include <bitaxe_handler.hpp>
#include <unity.h>

template <size_t N>
std::string joinArrayWithBrackets(const std::array<std::string, N> &arr,
                                  const std::string &separator = " ") {
  std::ostringstream result;
  for (size_t i = 0; i < N; ++i) {
    if (i > 0) {
      result << separator;
    }
    result << '[' << arr[i] << ']';
  }
  return result.str();
}

void setUp(void) {
  // set stuff up here
}

void tearDown(void) {
  // clean stuff up here
}

void test_BitaxeParseHashrate(void) {
  std::array<std::string, NUM_SCREENS> output =
      parseBitaxeHashRate(656130000000);

  std::string joined = joinArrayWithBrackets(output);

  TEST_ASSERT_EQUAL_STRING_MESSAGE("mdi:bitaxe", output[0].c_str(),
                                   joined.c_str());

  TEST_ASSERT_EQUAL_STRING_MESSAGE("6", output[NUM_SCREENS - 4].c_str(),
                                   joined.c_str());
  TEST_ASSERT_EQUAL_STRING_MESSAGE("5", output[NUM_SCREENS - 3].c_str(),
                                   joined.c_str());
  TEST_ASSERT_EQUAL_STRING_MESSAGE("6", output[NUM_SCREENS - 2].c_str(),
                                   joined.c_str());
  TEST_ASSERT_EQUAL_STRING_MESSAGE("GH/S", output[NUM_SCREENS - 1].c_str(),
                                   joined.c_str());
}

void test_BitaxeParseBestDiff(void) {
  std::array<std::string, NUM_SCREENS> output =
      parseBitaxeBestDiff(15800000000);

  std::string joined = joinArrayWithBrackets(output);

  TEST_ASSERT_EQUAL_STRING_MESSAGE("mdi:bitaxe", output[0].c_str(),
                                   joined.c_str());

  TEST_ASSERT_EQUAL_STRING_MESSAGE("1", output[NUM_SCREENS - 5].c_str(),
                                   joined.c_str());
  TEST_ASSERT_EQUAL_STRING_MESSAGE("5", output[NUM_SCREENS - 4].c_str(),
                                   joined.c_str());
  TEST_ASSERT_EQUAL_STRING_MESSAGE(".", output[NUM_SCREENS - 3].c_str(),
                                   joined.c_str());
  TEST_ASSERT_EQUAL_STRING_MESSAGE("8", output[NUM_SCREENS - 2].c_str(),
                                   joined.c_str());
  TEST_ASSERT_EQUAL_STRING_MESSAGE("G", output[NUM_SCREENS - 1].c_str(),
                                   joined.c_str());
}

void test_BitaxeParseHashrate_Zero(void) {
  // hashrate=0 -> "0" GH/s; must not underflow startIndex.
  std::array<std::string, NUM_SCREENS> output = parseBitaxeHashRate(0);
  TEST_ASSERT_EQUAL_STRING("mdi:bitaxe", output[0].c_str());
  TEST_ASSERT_EQUAL_STRING("GH/S", output[NUM_SCREENS - 1].c_str());
  TEST_ASSERT_EQUAL_STRING("0", output[NUM_SCREENS - 2].c_str());
}

void test_BitaxeParseHashrate_Huge(void) {
  // Regression: very large hashrate produces a 10+ digit GH string.
  // Previously size_t startIndex = NUM_SCREENS - 1 - textLength underflowed
  // to a huge value and the following loop wrote out-of-bounds. Must remain
  // in-bounds and not crash.
  std::array<std::string, NUM_SCREENS> output =
      parseBitaxeHashRate(1000000000000000000ULL); // 1e18 H/s
  TEST_ASSERT_EQUAL_STRING("mdi:bitaxe", output[0].c_str());
  TEST_ASSERT_EQUAL_STRING("GH/S", output[NUM_SCREENS - 1].c_str());
  // The remaining slots should be populated with digits (no empty holes).
  for (std::size_t i = 1; i < NUM_SCREENS - 1; ++i) {
    // It's fine to have "mdi:pickaxe" at one position, but nothing past
    // the end of the array should have been written.
    TEST_ASSERT_TRUE(!output[i].empty());
  }
}

void test_BitaxeParseBestDiff_Huge(void) {
  // Regression: diff > 10^15 (Q suffix). The resulting text can be "999Q" etc.
  // Must not underflow startIndex nor write OOB.
  std::array<std::string, NUM_SCREENS> output =
      parseBitaxeBestDiff(999000000000000000ULL); // ~999P
  TEST_ASSERT_EQUAL_STRING("mdi:bitaxe", output[0].c_str());
  TEST_ASSERT_EQUAL_STRING("mdi:rocket", output[1].c_str());
  // Last slot should be the suffix letter.
  TEST_ASSERT_TRUE_MESSAGE(output[NUM_SCREENS - 1].length() > 0,
                           output[NUM_SCREENS - 1].c_str());
}

// not needed when using generate_test_runner.rb
int runUnityTests(void) {
  UNITY_BEGIN();
  RUN_TEST(test_BitaxeParseHashrate);
  RUN_TEST(test_BitaxeParseBestDiff);
  RUN_TEST(test_BitaxeParseHashrate_Zero);
  RUN_TEST(test_BitaxeParseHashrate_Huge);
  RUN_TEST(test_BitaxeParseBestDiff_Huge);

  return UNITY_END();
}

int main(void) { return runUnityTests(); }

extern "C" void app_main() { runUnityTests(); }
