#include <dnd_window.hpp>
#include <unity.h>

using btclock::isTimeInDNDRange;

void setUp(void) {}
void tearDown(void) {}

// ---------------------------------------------------------------------------
// Same-day windows
// ---------------------------------------------------------------------------

void test_SameDay_InsideRange(void) {
  // 22:00 -> 23:00 window, current time 22:30 -> active.
  TEST_ASSERT_TRUE(isTimeInDNDRange(22, 30, 22, 0, 23, 0));
}

void test_SameDay_AtStart(void) {
  // Start boundary is inclusive.
  TEST_ASSERT_TRUE(isTimeInDNDRange(22, 0, 22, 0, 23, 0));
}

void test_SameDay_AtEnd(void) {
  // End boundary is exclusive.
  TEST_ASSERT_FALSE(isTimeInDNDRange(23, 0, 22, 0, 23, 0));
}

void test_SameDay_OutsideRange(void) {
  TEST_ASSERT_FALSE(isTimeInDNDRange(10, 0, 22, 0, 23, 0));
  TEST_ASSERT_FALSE(isTimeInDNDRange(23, 1, 22, 0, 23, 0));
}

// ---------------------------------------------------------------------------
// Midnight-wrap windows
// ---------------------------------------------------------------------------

void test_WrapAroundMidnight_BeforeMidnight(void) {
  // 22:30 -> 07:00 window, current time 22:31 -> active.
  TEST_ASSERT_TRUE(isTimeInDNDRange(22, 31, 22, 30, 7, 0));
}

void test_WrapAroundMidnight_Midnight(void) {
  TEST_ASSERT_TRUE(isTimeInDNDRange(0, 0, 22, 30, 7, 0));
}

void test_WrapAroundMidnight_AfterMidnight(void) {
  TEST_ASSERT_TRUE(isTimeInDNDRange(6, 59, 22, 30, 7, 0));
}

void test_WrapAroundMidnight_AtEndBoundary(void) {
  // End boundary stays exclusive across the wrap.
  TEST_ASSERT_FALSE(isTimeInDNDRange(7, 0, 22, 30, 7, 0));
}

void test_WrapAroundMidnight_WellOutside(void) {
  TEST_ASSERT_FALSE(isTimeInDNDRange(12, 0, 22, 30, 7, 0));
}

// ---------------------------------------------------------------------------
// Degenerate cases
// ---------------------------------------------------------------------------

void test_StartEqualsEnd_IsEmptyWindow(void) {
  // start == end is explicitly treated as "never active" so picking
  // the same start and end time cannot lock DND on forever.
  TEST_ASSERT_FALSE(isTimeInDNDRange(12, 0, 12, 0, 12, 0));
  TEST_ASSERT_FALSE(isTimeInDNDRange(0, 0, 12, 0, 12, 0));
  TEST_ASSERT_FALSE(isTimeInDNDRange(23, 59, 12, 0, 12, 0));
}

void test_OneMinuteWindow(void) {
  // Smallest non-empty window: exactly one minute.
  TEST_ASSERT_TRUE(isTimeInDNDRange(12, 0, 12, 0, 12, 1));
  TEST_ASSERT_FALSE(isTimeInDNDRange(12, 1, 12, 0, 12, 1));
  TEST_ASSERT_FALSE(isTimeInDNDRange(11, 59, 12, 0, 12, 1));
}

void test_WholeDayMinusOne(void) {
  // 00:00 -> 23:59 — everything except 23:59 should be DND.
  TEST_ASSERT_TRUE(isTimeInDNDRange(0, 0, 0, 0, 23, 59));
  TEST_ASSERT_TRUE(isTimeInDNDRange(12, 0, 0, 0, 23, 59));
  TEST_ASSERT_FALSE(isTimeInDNDRange(23, 59, 0, 0, 23, 59));
}

// ---------------------------------------------------------------------------

int runUnityTests(void) {
  UNITY_BEGIN();
  RUN_TEST(test_SameDay_InsideRange);
  RUN_TEST(test_SameDay_AtStart);
  RUN_TEST(test_SameDay_AtEnd);
  RUN_TEST(test_SameDay_OutsideRange);
  RUN_TEST(test_WrapAroundMidnight_BeforeMidnight);
  RUN_TEST(test_WrapAroundMidnight_Midnight);
  RUN_TEST(test_WrapAroundMidnight_AfterMidnight);
  RUN_TEST(test_WrapAroundMidnight_AtEndBoundary);
  RUN_TEST(test_WrapAroundMidnight_WellOutside);
  RUN_TEST(test_StartEqualsEnd_IsEmptyWindow);
  RUN_TEST(test_OneMinuteWindow);
  RUN_TEST(test_WholeDayMinusOne);
  return UNITY_END();
}

int main(void) { return runUnityTests(); }

extern "C" void app_main() { runUnityTests(); }
