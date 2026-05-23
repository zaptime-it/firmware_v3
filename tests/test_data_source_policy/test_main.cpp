// Tests for the data-source flash-suppression policy.
//
// The real LED / WiFi stacks aren't exercised here — that would require
// hardware or a non-trivial mock layer. The predicate itself is a pure
// function whose only purpose is to lock in the invariant "never fire
// the data-source disconnect LED effect while WiFi is known to be down."
// A regression that flips either half of the gate silently reintroduces
// the flash storm users report when WiFi drops.

#include "../../main/lib/data_sources/data_source_policy.hpp"
#include <unity.h>

using data_source_policy::shouldFlashDataSourceError;

void setUp(void) {}
void tearDown(void) {}

void test_firstDisconnectIsSilent_evenWhenWifiIsUp(void) {
  // The original behavior only flashes after disconnectCount > 1, so the
  // very first disconnect of a session is debounced. Preserve that.
  TEST_ASSERT_FALSE(shouldFlashDataSourceError(1, true));
}

void test_repeatDisconnectFlashesWhenWifiIsUp(void) {
  // The common case: WiFi is fine, the upstream is flaky. The purple
  // disconnect blink is actually informative here, so keep firing it.
  TEST_ASSERT_TRUE(shouldFlashDataSourceError(2, true));
  TEST_ASSERT_TRUE(shouldFlashDataSourceError(10, true));
}

void test_wifiDownSuppressesTheDataSourceFlash(void) {
  // The regression guard. If a future change drops the WiFi check, this
  // test fails and the "flash storm on WiFi drop" bug is back.
  TEST_ASSERT_FALSE(shouldFlashDataSourceError(2, false));
  TEST_ASSERT_FALSE(shouldFlashDataSourceError(100, false));
}

void test_zeroDisconnectsNeverFlash(void) {
  // Defensive — the counter starts at 0. Shouldn't flash regardless of
  // WiFi state.
  TEST_ASSERT_FALSE(shouldFlashDataSourceError(0, true));
  TEST_ASSERT_FALSE(shouldFlashDataSourceError(0, false));
}

int runUnityTests(void) {
  UNITY_BEGIN();
  RUN_TEST(test_firstDisconnectIsSilent_evenWhenWifiIsUp);
  RUN_TEST(test_repeatDisconnectFlashesWhenWifiIsUp);
  RUN_TEST(test_wifiDownSuppressesTheDataSourceFlash);
  RUN_TEST(test_zeroDisconnectsNeverFlash);
  return UNITY_END();
}

int main(void) { return runUnityTests(); }

extern "C" void app_main() { runUnityTests(); }
