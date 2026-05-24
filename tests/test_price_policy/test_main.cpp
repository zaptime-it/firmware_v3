// Tests for the Kraken-subscription currency-list parser.
//
// Background: the Kraken v2 ticker WS accepts a JSON array of symbols
// in a single subscribe frame. The firmware builds that array from the
// user-configured actCurrencies CSV. The subscription-building code used
// to hardcode "BTC/USD" only; after extending it to all actCurrencies,
// these tests lock in the exact CSV-parsing invariants that the
// subscription frame depends on, so a whitespace or empty-token
// regression can't silently reintroduce "only USD".

#include "../../main/lib/data_sources/price_policy.hpp"
#include <unity.h>

using price_policy::parseCurrencyCsv;

void setUp(void) {}
void tearDown(void) {}

void test_singleCurrencyStaysSingle(void) {
  auto got = parseCurrencyCsv("USD");
  TEST_ASSERT_EQUAL_UINT(1, got.size());
  TEST_ASSERT_EQUAL_STRING("USD", got[0].c_str());
}

void test_multiCurrencyPreservesOrder(void) {
  auto got = parseCurrencyCsv("USD,EUR,JPY");
  TEST_ASSERT_EQUAL_UINT(3, got.size());
  TEST_ASSERT_EQUAL_STRING("USD", got[0].c_str());
  TEST_ASSERT_EQUAL_STRING("EUR", got[1].c_str());
  TEST_ASSERT_EQUAL_STRING("JPY", got[2].c_str());
}

void test_surroundingWhitespaceIsTrimmed(void) {
  // The WebUI currently packs without spaces, but nothing prevents a
  // user from hand-editing the preference — and Kraken would reject
  // " BTC/USD".
  auto got = parseCurrencyCsv(" USD , EUR , JPY ");
  TEST_ASSERT_EQUAL_UINT(3, got.size());
  TEST_ASSERT_EQUAL_STRING("USD", got[0].c_str());
  TEST_ASSERT_EQUAL_STRING("EUR", got[1].c_str());
  TEST_ASSERT_EQUAL_STRING("JPY", got[2].c_str());
}

void test_trailingAndDoubleCommasDropEmpties(void) {
  // "USD,,EUR," previously produced empty "BTC/" entries that Kraken
  // would reject, failing the entire subscription.
  auto got = parseCurrencyCsv("USD,,EUR,");
  TEST_ASSERT_EQUAL_UINT(2, got.size());
  TEST_ASSERT_EQUAL_STRING("USD", got[0].c_str());
  TEST_ASSERT_EQUAL_STRING("EUR", got[1].c_str());
}

void test_emptyStringYieldsEmptyList(void) {
  // The caller is responsible for falling back to USD when the list
  // is empty; the parser stays pure.
  auto got = parseCurrencyCsv("");
  TEST_ASSERT_EQUAL_UINT(0, got.size());
}

void test_whitespaceOnlyYieldsEmptyList(void) {
  auto got = parseCurrencyCsv("   ");
  TEST_ASSERT_EQUAL_UINT(0, got.size());
}

int runUnityTests(void) {
  UNITY_BEGIN();
  RUN_TEST(test_singleCurrencyStaysSingle);
  RUN_TEST(test_multiCurrencyPreservesOrder);
  RUN_TEST(test_surroundingWhitespaceIsTrimmed);
  RUN_TEST(test_trailingAndDoubleCommasDropEmpties);
  RUN_TEST(test_emptyStringYieldsEmptyList);
  RUN_TEST(test_whitespaceOnlyYieldsEmptyList);
  return UNITY_END();
}

int main(void) { return runUnityTests(); }

extern "C" void app_main() { runUnityTests(); }
