#include <utils.hpp>
#include <unity.h>

void test_parseMiningPoolStatsHashRate1dot34TH(void)
{
    std::string hashrate;
    std::string label;
    std::string output;
  
    parseHashrateString("1340000000000", label, output, 4);

    TEST_ASSERT_EQUAL_STRING_MESSAGE("TH/S", label.c_str(), label.c_str()); 
    TEST_ASSERT_EQUAL_STRING_MESSAGE("1.34", output.c_str(), output.c_str());
}

void test_parseMiningPoolStatsHashRate645GH(void)
{
    std::string hashrate = "645000000000";
    std::string label;
    std::string output;
  
    parseHashrateString(hashrate, label, output, 4);

    TEST_ASSERT_EQUAL_STRING_MESSAGE("GH/S", label.c_str(), label.c_str()); 
    TEST_ASSERT_EQUAL_STRING_MESSAGE("645", output.c_str(), output.c_str());
}

void test_parseMiningPoolStatsHashRateEmpty(void)
{
    std::string hashrate = "";
    std::string label;
    std::string output;
  
    parseHashrateString(hashrate, label, output, 4);

    TEST_ASSERT_EQUAL_STRING_MESSAGE("H/S", label.c_str(), label.c_str()); 
    TEST_ASSERT_EQUAL_STRING_MESSAGE("0", output.c_str(), output.c_str());
}

void test_parseMiningPoolStatsHashRateZero(void)
{
    std::string hashrate = "0";
    std::string label;
    std::string output;
  
    parseHashrateString(hashrate, label, output, 4);

    TEST_ASSERT_EQUAL_STRING_MESSAGE("H/S", label.c_str(), label.c_str()); 
    TEST_ASSERT_EQUAL_STRING_MESSAGE("0", output.c_str(), output.c_str());
}




// not needed when using generate_test_runner.rb
int runUnityTests(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_parseMiningPoolStatsHashRate1dot34TH);
    RUN_TEST(test_parseMiningPoolStatsHashRate645GH);
    RUN_TEST(test_parseMiningPoolStatsHashRateZero);
    RUN_TEST(test_parseMiningPoolStatsHashRateEmpty);

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
