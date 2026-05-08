#include <screen_nav.hpp>
#include <unity.h>

using btclock::nextCurrencyIndex;

void setUp(void) {}
void tearDown(void) {}

// ---------------------------------------------------------------------------
// Non-currency-specific target screens: never reset currency.
// ---------------------------------------------------------------------------

void test_TargetNonCurrencySpecific_Forward_NoReset(void) {
  TEST_ASSERT_EQUAL_INT(-1, nextCurrencyIndex(false, true, 3));
}

void test_TargetNonCurrencySpecific_Backward_NoReset(void) {
  TEST_ASSERT_EQUAL_INT(-1, nextCurrencyIndex(false, false, 3));
}

// ---------------------------------------------------------------------------
// Currency-specific target screens: reset to first (forward) or last (back).
// ---------------------------------------------------------------------------

void test_TargetCurrencySpecific_Forward_ResetsToFirst(void) {
  TEST_ASSERT_EQUAL_INT(0, nextCurrencyIndex(true, true, 3));
}

void test_TargetCurrencySpecific_Backward_ResetsToLast(void) {
  TEST_ASSERT_EQUAL_INT(2, nextCurrencyIndex(true, false, 3));
}

void test_TargetCurrencySpecific_SingleCurrency_Forward(void) {
  TEST_ASSERT_EQUAL_INT(0, nextCurrencyIndex(true, true, 1));
}

void test_TargetCurrencySpecific_SingleCurrency_Backward(void) {
  TEST_ASSERT_EQUAL_INT(0, nextCurrencyIndex(true, false, 1));
}

// ---------------------------------------------------------------------------
// Defensive: empty / negative currency list.
// ---------------------------------------------------------------------------

void test_EmptyCurrencyList_Forward_NoReset(void) {
  TEST_ASSERT_EQUAL_INT(-1, nextCurrencyIndex(true, true, 0));
}

void test_EmptyCurrencyList_Backward_NoReset(void) {
  TEST_ASSERT_EQUAL_INT(-1, nextCurrencyIndex(true, false, 0));
}

void test_NegativeCurrencyCount_NoReset(void) {
  TEST_ASSERT_EQUAL_INT(-1, nextCurrencyIndex(true, true, -1));
}

// ---------------------------------------------------------------------------
// Regression: the "prev from first currency, then next again" sequence.
//
// The old code reset currency based on the *leaving* screen. Leaving a
// cs-screen going backward stamped currency = last, then landing forward
// on a cs-screen left it at last instead of first. Simulate the two
// transitions with the new target-screen-based logic and confirm that
// the forward transition back onto the cs-screen produces index 0.
// ---------------------------------------------------------------------------

void test_Regression_PrevThenNext_LandsOnFirstCurrency(void) {
  const int activeCurrencies = 3;

  // Press previous: cs-screen (first currency) -> non-cs screen.
  // Target is non-cs, so currency must not be touched.
  int afterPrev = nextCurrencyIndex(/*newScreenIsCurrencySpecific=*/false,
                                    /*forward=*/false, activeCurrencies);
  TEST_ASSERT_EQUAL_INT(-1, afterPrev);

  // Press next: non-cs screen -> cs-screen. Target is cs, forward -> 0.
  int afterNext = nextCurrencyIndex(/*newScreenIsCurrencySpecific=*/true,
                                    /*forward=*/true, activeCurrencies);
  TEST_ASSERT_EQUAL_INT(0, afterNext);
}

// Symmetric: next off a cs-screen at last currency, then prev back
// onto the cs-screen should land on the last currency.
void test_Regression_NextThenPrev_LandsOnLastCurrency(void) {
  const int activeCurrencies = 3;

  int afterNext = nextCurrencyIndex(/*newScreenIsCurrencySpecific=*/false,
                                    /*forward=*/true, activeCurrencies);
  TEST_ASSERT_EQUAL_INT(-1, afterNext);

  int afterPrev = nextCurrencyIndex(/*newScreenIsCurrencySpecific=*/true,
                                    /*forward=*/false, activeCurrencies);
  TEST_ASSERT_EQUAL_INT(2, afterPrev);
}

// ---------------------------------------------------------------------------

int runUnityTests(void) {
  UNITY_BEGIN();
  RUN_TEST(test_TargetNonCurrencySpecific_Forward_NoReset);
  RUN_TEST(test_TargetNonCurrencySpecific_Backward_NoReset);
  RUN_TEST(test_TargetCurrencySpecific_Forward_ResetsToFirst);
  RUN_TEST(test_TargetCurrencySpecific_Backward_ResetsToLast);
  RUN_TEST(test_TargetCurrencySpecific_SingleCurrency_Forward);
  RUN_TEST(test_TargetCurrencySpecific_SingleCurrency_Backward);
  RUN_TEST(test_EmptyCurrencyList_Forward_NoReset);
  RUN_TEST(test_EmptyCurrencyList_Backward_NoReset);
  RUN_TEST(test_NegativeCurrencyCount_NoReset);
  RUN_TEST(test_Regression_PrevThenNext_LandsOnFirstCurrency);
  RUN_TEST(test_Regression_NextThenPrev_LandsOnLastCurrency);
  return UNITY_END();
}

int main(void) { return runUnityTests(); }

extern "C" void app_main() { runUnityTests(); }
