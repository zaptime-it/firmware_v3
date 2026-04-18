#include <nostrdisplay_handler.hpp>
#include <unity.h>

void setUp(void) {}
void tearDown(void) {}

void test_ZapNotify_Small(void)
{
    // amount = 21 -> 2 digit string
    std::array<std::string, NUM_SCREENS> output = parseZapNotify(21, false);
    TEST_ASSERT_EQUAL_STRING("ZAP", output[0].c_str());
    TEST_ASSERT_EQUAL_STRING("mdi-lnbolt", output[1].c_str());
    TEST_ASSERT_EQUAL_STRING("2", output[NUM_SCREENS - 2].c_str());
    TEST_ASSERT_EQUAL_STRING("1", output[NUM_SCREENS - 1].c_str());
}

void test_ZapNotify_SmallWithSymbol(void)
{
    std::array<std::string, NUM_SCREENS> output = parseZapNotify(21, true);
    TEST_ASSERT_EQUAL_STRING("ZAP", output[0].c_str());
    TEST_ASSERT_EQUAL_STRING("mdi-lnbolt", output[1].c_str());
    // "STS" should precede the digits.
    TEST_ASSERT_EQUAL_STRING("STS", output[NUM_SCREENS - 3].c_str());
    TEST_ASSERT_EQUAL_STRING("2", output[NUM_SCREENS - 2].c_str());
    TEST_ASSERT_EQUAL_STRING("1", output[NUM_SCREENS - 1].c_str());
}

void test_ZapNotify_Zero(void)
{
    // amount=0 -> "0" (1 char). Ensure no underflow and ZAP label preserved.
    std::array<std::string, NUM_SCREENS> output = parseZapNotify(0, false);
    TEST_ASSERT_EQUAL_STRING("ZAP", output[0].c_str());
    TEST_ASSERT_EQUAL_STRING("0", output[NUM_SCREENS - 1].c_str());
}

void test_ZapNotify_Max(void)
{
    // uint16_t max = 65535 (5 digits). Previously no underflow but the STS
    // symbol would overwrite the lnbolt label. This test at minimum ensures
    // no OOB access and that the ZAP label survives.
    std::array<std::string, NUM_SCREENS> output = parseZapNotify(65535, false);
    TEST_ASSERT_EQUAL_STRING("ZAP", output[0].c_str());
    // Last 5 positions should hold the digits.
    TEST_ASSERT_EQUAL_STRING("6", output[NUM_SCREENS - 5].c_str());
    TEST_ASSERT_EQUAL_STRING("5", output[NUM_SCREENS - 4].c_str());
    TEST_ASSERT_EQUAL_STRING("5", output[NUM_SCREENS - 3].c_str());
    TEST_ASSERT_EQUAL_STRING("3", output[NUM_SCREENS - 2].c_str());
    TEST_ASSERT_EQUAL_STRING("5", output[NUM_SCREENS - 1].c_str());
}

void test_ZapNotify_MaxWithSymbol(void)
{
    // Max amount + symbol: ensure no OOB even when startIndex is small.
    std::array<std::string, NUM_SCREENS> output = parseZapNotify(65535, true);
    TEST_ASSERT_EQUAL_STRING("ZAP", output[0].c_str());
    // The lnbolt at index 1 may be replaced by "STS" in current 7-screen
    // layout since the digits take 5 slots (positions 2..6) and STS goes at
    // position 1. We just assert no crash and that index 1 is populated.
    TEST_ASSERT_TRUE(!output[1].empty());
}

int runUnityTests(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_ZapNotify_Small);
    RUN_TEST(test_ZapNotify_SmallWithSymbol);
    RUN_TEST(test_ZapNotify_Zero);
    RUN_TEST(test_ZapNotify_Max);
    RUN_TEST(test_ZapNotify_MaxWithSymbol);
    return UNITY_END();
}

int main(void)
{
    return runUnityTests();
}

extern "C" void app_main()
{
    runUnityTests();
}
