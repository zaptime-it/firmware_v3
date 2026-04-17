#include <utils.hpp>
#include <unity.h>

void setUp(void) {}
void tearDown(void) {}

// ---------------------------------------------------------------------------
// formatNumberWithSuffix
// ---------------------------------------------------------------------------

void test_FormatNumberWithSuffix_Zero(void)
{
    // Regression: log10(0) is -inf; casting to int is undefined behaviour.
    // formatNumberWithSuffix(0) must return a sane "0" for both mow and !mow modes.
    std::string out = formatNumberWithSuffix(0, 4, false);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("0", out.c_str(), out.c_str());

    std::string outMow = formatNumberWithSuffix(0, 4, true);
    // mowMode falls into the "< thousand" else-branch and divides 0/million => "0M" (or similar).
    // We only assert non-empty and no crash.
    TEST_ASSERT_TRUE_MESSAGE(!outMow.empty(), outMow.c_str());
}

void test_FormatNumberWithSuffix_One(void)
{
    std::string out = formatNumberWithSuffix(1, 4, false);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("1", out.c_str(), out.c_str());
}

void test_FormatNumberWithSuffix_Thousand(void)
{
    // numCharacters=4 -> "1.0K" (4 chars incl. suffix)
    std::string out = formatNumberWithSuffix(1000, 4, false);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("1.0K", out.c_str(), out.c_str());
}

void test_FormatNumberWithSuffix_Million(void)
{
    std::string out = formatNumberWithSuffix(1500000ULL, 4, false);
    // "2M" (4 chars available -> integer part + suffix)
    // With numCharacters=4 and "2M" length 2 -> restLen = 4-2-1 = 1 -> "%.1f%c" -> "1.5M"
    TEST_ASSERT_EQUAL_STRING_MESSAGE("1.5M", out.c_str(), out.c_str());
}

void test_FormatNumberWithSuffix_Billion(void)
{
    std::string out = formatNumberWithSuffix(1500000000ULL, 4, false);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("1.5B", out.c_str(), out.c_str());
}

// ---------------------------------------------------------------------------
// getHashrateMultiplier / getDifficultyMultiplier
// ---------------------------------------------------------------------------

void test_GetHashrateMultiplier_Known(void)
{
    TEST_ASSERT_EQUAL_INT(0, getHashrateMultiplier('0'));
    TEST_ASSERT_EQUAL_INT(3, getHashrateMultiplier('K'));
    TEST_ASSERT_EQUAL_INT(6, getHashrateMultiplier('M'));
    TEST_ASSERT_EQUAL_INT(9, getHashrateMultiplier('G'));
    TEST_ASSERT_EQUAL_INT(12, getHashrateMultiplier('T'));
    TEST_ASSERT_EQUAL_INT(15, getHashrateMultiplier('P'));
    TEST_ASSERT_EQUAL_INT(18, getHashrateMultiplier('E'));
    TEST_ASSERT_EQUAL_INT(21, getHashrateMultiplier('Z'));
}

void test_GetHashrateMultiplier_Unknown(void)
{
    // Regression: multipliers.at(unknown) previously threw std::out_of_range.
    // The function must now return 0 for unknown characters.
    TEST_ASSERT_EQUAL_INT(0, getHashrateMultiplier('X'));
    TEST_ASSERT_EQUAL_INT(0, getHashrateMultiplier(' '));
    TEST_ASSERT_EQUAL_INT(0, getHashrateMultiplier('!'));
}

void test_GetDifficultyMultiplier_Known(void)
{
    TEST_ASSERT_EQUAL_INT(0, getDifficultyMultiplier('0'));
    TEST_ASSERT_EQUAL_INT(3, getDifficultyMultiplier('K'));
    TEST_ASSERT_EQUAL_INT(3, getDifficultyMultiplier('k'));
    TEST_ASSERT_EQUAL_INT(6, getDifficultyMultiplier('M'));
    TEST_ASSERT_EQUAL_INT(9, getDifficultyMultiplier('G'));
    TEST_ASSERT_EQUAL_INT(9, getDifficultyMultiplier('B'));
    TEST_ASSERT_EQUAL_INT(12, getDifficultyMultiplier('T'));
    TEST_ASSERT_EQUAL_INT(15, getDifficultyMultiplier('Q'));
}

void test_GetDifficultyMultiplier_Unknown(void)
{
    // Regression: must not throw on unknown unit.
    TEST_ASSERT_EQUAL_INT(0, getDifficultyMultiplier('X'));
    TEST_ASSERT_EQUAL_INT(0, getDifficultyMultiplier(' '));
}

// ---------------------------------------------------------------------------
// parseHashrateString
// ---------------------------------------------------------------------------

void test_ParseHashrateString_Empty(void)
{
    std::string label, output;
    parseHashrateString("", label, output, 4);
    TEST_ASSERT_EQUAL_STRING("H/S", label.c_str());
    TEST_ASSERT_EQUAL_STRING("0", output.c_str());
}

void test_ParseHashrateString_InvalidInput(void)
{
    // Regression: std::stod("abc") previously threw std::invalid_argument and
    // crashed the runtime. Must fall back to 0/H/S.
    std::string label, output;
    parseHashrateString("not-a-number", label, output, 4);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("H/S", label.c_str(), label.c_str());
    TEST_ASSERT_EQUAL_STRING_MESSAGE("0", output.c_str(), output.c_str());
}

void test_ParseHashrateString_GarbageWithDigit(void)
{
    // "123abc" previously parsed as 123 via stod's partial-parse.
    // Acceptable behaviour: treat as 123 H/S (or fall back to 0). Ensure no throw.
    std::string label, output;
    parseHashrateString("123abc", label, output, 4);
    TEST_ASSERT_FALSE_MESSAGE(label.empty(), label.c_str());
    TEST_ASSERT_FALSE_MESSAGE(output.empty(), output.c_str());
}

void test_ParseHashrateString_Huge(void)
{
    // Very long numeric string (> 21 digits) -> ZH/S branch.
    std::string label, output;
    parseHashrateString("123456789012345678901234", label, output, 4);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("ZH/S", label.c_str(), label.c_str());
    // Expected: integer part "123" (3 digits), no space for decimals -> "123"
    TEST_ASSERT_EQUAL_STRING_MESSAGE("123", output.c_str(), output.c_str());
}

// ---------------------------------------------------------------------------
// getAmountInSatoshis (bolt11 parser)
// ---------------------------------------------------------------------------

void test_GetAmountInSatoshis_Milli(void)
{
    // 1m BTC = 0.001 BTC = 100,000 sats
    TEST_ASSERT_EQUAL_INT64(100000, getAmountInSatoshis("lnbc1m..."));
}

void test_GetAmountInSatoshis_Invalid(void)
{
    TEST_ASSERT_EQUAL_INT64(-1, getAmountInSatoshis(""));
    TEST_ASSERT_EQUAL_INT64(-1, getAmountInSatoshis("no-digits-here"));
}

// ---------------------------------------------------------------------------
// getSupplyAtBlock
// ---------------------------------------------------------------------------

void test_GetSupplyAtBlock_Genesis(void)
{
    // Unity is built without double precision assertions; compare as float.
    TEST_ASSERT_EQUAL_FLOAT(0.0f, (float)getSupplyAtBlock(0));
}

void test_GetSupplyAtBlock_Cap(void)
{
    // Well past the 33rd halving -> full supply
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 21000000.0f, (float)getSupplyAtBlock(34 * 210000));
}

// ---------------------------------------------------------------------------

int runUnityTests(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_FormatNumberWithSuffix_Zero);
    RUN_TEST(test_FormatNumberWithSuffix_One);
    RUN_TEST(test_FormatNumberWithSuffix_Thousand);
    RUN_TEST(test_FormatNumberWithSuffix_Million);
    RUN_TEST(test_FormatNumberWithSuffix_Billion);
    RUN_TEST(test_GetHashrateMultiplier_Known);
    RUN_TEST(test_GetHashrateMultiplier_Unknown);
    RUN_TEST(test_GetDifficultyMultiplier_Known);
    RUN_TEST(test_GetDifficultyMultiplier_Unknown);
    RUN_TEST(test_ParseHashrateString_Empty);
    RUN_TEST(test_ParseHashrateString_InvalidInput);
    RUN_TEST(test_ParseHashrateString_GarbageWithDigit);
    RUN_TEST(test_ParseHashrateString_Huge);
    RUN_TEST(test_GetAmountInSatoshis_Milli);
    RUN_TEST(test_GetAmountInSatoshis_Invalid);
    RUN_TEST(test_GetSupplyAtBlock_Genesis);
    RUN_TEST(test_GetSupplyAtBlock_Cap);
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
