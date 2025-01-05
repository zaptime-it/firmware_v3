#include <bitaxe_handler.hpp>
#include <unity.h>

template<size_t N>
std::string joinArrayWithBrackets(const std::array<std::string, N>& arr, const std::string& separator = " ") {
    std::ostringstream result;
    for (size_t i = 0; i < N; ++i) {
        if (i > 0) {
            result << separator;
        }
        result << '[' << arr[i] << ']';
    }
    return result.str();
}

void setUp(void)
{
    // set stuff up here
}

void tearDown(void)
{
    // clean stuff up here
}

void test_BitaxeParseHashrate(void)
{
    std::array<std::string, NUM_SCREENS> output = parseBitaxeHashRate(656130000000);

    std::string joined = joinArrayWithBrackets(output);


    TEST_ASSERT_EQUAL_STRING_MESSAGE("mdi:bitaxe", output[0].c_str(), joined.c_str());

    TEST_ASSERT_EQUAL_STRING_MESSAGE("6", output[NUM_SCREENS - 4].c_str(), joined.c_str());
    TEST_ASSERT_EQUAL_STRING_MESSAGE("5", output[NUM_SCREENS - 3].c_str(), joined.c_str());
    TEST_ASSERT_EQUAL_STRING_MESSAGE("6", output[NUM_SCREENS - 2].c_str(), joined.c_str());
    TEST_ASSERT_EQUAL_STRING_MESSAGE("GH/S", output[NUM_SCREENS - 1].c_str(), joined.c_str());
}

void test_BitaxeParseBestDiff(void)
{
    std::array<std::string, NUM_SCREENS> output = parseBitaxeBestDiff(15800000000);

    std::string joined = joinArrayWithBrackets(output);

    TEST_ASSERT_EQUAL_STRING_MESSAGE("mdi:bitaxe", output[0].c_str(), joined.c_str());


    TEST_ASSERT_EQUAL_STRING_MESSAGE("1", output[NUM_SCREENS - 5].c_str(), joined.c_str());
    TEST_ASSERT_EQUAL_STRING_MESSAGE("5", output[NUM_SCREENS - 4].c_str(), joined.c_str());
    TEST_ASSERT_EQUAL_STRING_MESSAGE(".", output[NUM_SCREENS - 3].c_str(), joined.c_str());
    TEST_ASSERT_EQUAL_STRING_MESSAGE("8", output[NUM_SCREENS - 2].c_str(), joined.c_str());
    TEST_ASSERT_EQUAL_STRING_MESSAGE("G", output[NUM_SCREENS - 1].c_str(), joined.c_str());
}

// not needed when using generate_test_runner.rb
int runUnityTests(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_BitaxeParseHashrate);
    RUN_TEST(test_BitaxeParseBestDiff);

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
