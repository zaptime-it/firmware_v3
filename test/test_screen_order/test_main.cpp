// test_screen_order / test_main.cpp
//
// Unit coverage for the pure parse + catalog-merge logic that backs the
// user-configurable rotation order. These functions live in lib/btclock/
// so they link natively without pulling in Arduino, Preferences, or the
// ScreenHandler stack — the same pattern test_screen_nav uses.
//
// The firmware's setupPreferences() and the /api/settings PATCH handler
// both delegate to mergeScreenOrder() for the upgrade-path behaviour
// (unknown-ID drop, new-ID append, dedupe). If any of these invariants
// ever regresses, a user who's set a custom order will silently lose
// screens on their device — so every rule has a dedicated test below.

#include <screen_order.hpp>
#include <unity.h>

#include <string>
#include <vector>

using btclock::mergeScreenOrder;
using btclock::parseScreenOrder;
using btclock::serializeScreenOrder;

void setUp(void) {}
void tearDown(void) {}

// ---------------------------------------------------------------------------
// parseScreenOrder
// ---------------------------------------------------------------------------

void test_Parse_Happy(void) {
    auto result = parseScreenOrder("0,3,4");
    TEST_ASSERT_EQUAL_size_t(3, result.size());
    TEST_ASSERT_EQUAL_INT(0, result[0]);
    TEST_ASSERT_EQUAL_INT(3, result[1]);
    TEST_ASSERT_EQUAL_INT(4, result[2]);
}

void test_Parse_Empty(void) {
    auto result = parseScreenOrder("");
    TEST_ASSERT_EQUAL_size_t(0, result.size());
}

void test_Parse_Whitespace_Trimmed(void) {
    auto result = parseScreenOrder(" 0 , 3 ,  4");
    TEST_ASSERT_EQUAL_size_t(3, result.size());
    TEST_ASSERT_EQUAL_INT(0, result[0]);
    TEST_ASSERT_EQUAL_INT(3, result[1]);
    TEST_ASSERT_EQUAL_INT(4, result[2]);
}

void test_Parse_TrailingComma_SkippedNotError(void) {
    auto result = parseScreenOrder("0,3,");
    TEST_ASSERT_EQUAL_size_t(2, result.size());
    TEST_ASSERT_EQUAL_INT(0, result[0]);
    TEST_ASSERT_EQUAL_INT(3, result[1]);
}

void test_Parse_LeadingComma_SkippedNotError(void) {
    auto result = parseScreenOrder(",0,3");
    TEST_ASSERT_EQUAL_size_t(2, result.size());
    TEST_ASSERT_EQUAL_INT(0, result[0]);
    TEST_ASSERT_EQUAL_INT(3, result[1]);
}

void test_Parse_MalformedToken_Dropped(void) {
    // "abc" is not a valid int; parseIntStrict rejects it without throwing.
    // "42abc" must also be rejected — std::stoi otherwise silently reads 42.
    auto result = parseScreenOrder("0,abc,3,42abc,6");
    TEST_ASSERT_EQUAL_size_t(3, result.size());
    TEST_ASSERT_EQUAL_INT(0, result[0]);
    TEST_ASSERT_EQUAL_INT(3, result[1]);
    TEST_ASSERT_EQUAL_INT(6, result[2]);
}

void test_Parse_OnlyWhitespace_Empty(void) {
    auto result = parseScreenOrder("   ,   ");
    TEST_ASSERT_EQUAL_size_t(0, result.size());
}

// ---------------------------------------------------------------------------
// serializeScreenOrder
// ---------------------------------------------------------------------------

void test_Serialize_Empty(void) {
    TEST_ASSERT_EQUAL_STRING("", serializeScreenOrder({}).c_str());
}

void test_Serialize_Single(void) {
    TEST_ASSERT_EQUAL_STRING("7", serializeScreenOrder({7}).c_str());
}

void test_Serialize_RoundTrip(void) {
    const std::vector<int> in = {0, 3, 4, 10, 20};
    auto s = serializeScreenOrder(in);
    TEST_ASSERT_EQUAL_STRING("0,3,4,10,20", s.c_str());
    auto back = parseScreenOrder(s);
    TEST_ASSERT_EQUAL_size_t(in.size(), back.size());
    for (size_t i = 0; i < in.size(); ++i) {
        TEST_ASSERT_EQUAL_INT(in[i], back[i]);
    }
}

// ---------------------------------------------------------------------------
// mergeScreenOrder
// ---------------------------------------------------------------------------

void test_Merge_KnownIds_StoredOrderWins(void) {
    std::vector<int> stored = {3, 0};
    std::vector<int> catalog = {0, 3};
    auto result = mergeScreenOrder(stored, catalog);
    TEST_ASSERT_EQUAL_size_t(2, result.size());
    TEST_ASSERT_EQUAL_INT(3, result[0]);
    TEST_ASSERT_EQUAL_INT(0, result[1]);
}

void test_Merge_UnknownIdDropped(void) {
    // 99 is not in the current firmware's catalog (removed screen, or
    // user loaded a stored order that references something this build
    // doesn't support). Must drop it, not error.
    std::vector<int> stored = {3, 99, 0};
    std::vector<int> catalog = {0, 3};
    auto result = mergeScreenOrder(stored, catalog);
    TEST_ASSERT_EQUAL_size_t(2, result.size());
    TEST_ASSERT_EQUAL_INT(3, result[0]);
    TEST_ASSERT_EQUAL_INT(0, result[1]);
}

void test_Merge_NewIdAppended(void) {
    // A catalog entry the user has never seen (e.g. firmware update adds
    // a new screen type) should appear at the end of rotation, not at the
    // front — otherwise every update would reshuffle the user's order.
    std::vector<int> stored = {3};
    std::vector<int> catalog = {0, 3, 40};
    auto result = mergeScreenOrder(stored, catalog);
    TEST_ASSERT_EQUAL_size_t(3, result.size());
    TEST_ASSERT_EQUAL_INT(3, result[0]);
    TEST_ASSERT_EQUAL_INT(0, result[1]);
    TEST_ASSERT_EQUAL_INT(40, result[2]);
}

void test_Merge_NewIdsPreserveCatalogOrder(void) {
    std::vector<int> stored = {3};
    std::vector<int> catalog = {0, 3, 40, 70, 80};
    auto result = mergeScreenOrder(stored, catalog);
    TEST_ASSERT_EQUAL_size_t(5, result.size());
    TEST_ASSERT_EQUAL_INT(3, result[0]);
    TEST_ASSERT_EQUAL_INT(0, result[1]);
    TEST_ASSERT_EQUAL_INT(40, result[2]);
    TEST_ASSERT_EQUAL_INT(70, result[3]);
    TEST_ASSERT_EQUAL_INT(80, result[4]);
}

void test_Merge_DuplicateStored_FirstWins(void) {
    // A corrupt or hand-edited NVS value could list the same ID twice.
    // Keeping the first position (rather than e.g. the last) matches the
    // user's apparent intent from the leftmost occurrence.
    std::vector<int> stored = {3, 3, 0};
    std::vector<int> catalog = {0, 3};
    auto result = mergeScreenOrder(stored, catalog);
    TEST_ASSERT_EQUAL_size_t(2, result.size());
    TEST_ASSERT_EQUAL_INT(3, result[0]);
    TEST_ASSERT_EQUAL_INT(0, result[1]);
}

void test_Merge_EmptyStored_FullCatalog(void) {
    std::vector<int> stored = {};
    std::vector<int> catalog = {0, 3, 4};
    auto result = mergeScreenOrder(stored, catalog);
    TEST_ASSERT_EQUAL_size_t(3, result.size());
    TEST_ASSERT_EQUAL_INT(0, result[0]);
    TEST_ASSERT_EQUAL_INT(3, result[1]);
    TEST_ASSERT_EQUAL_INT(4, result[2]);
}

void test_Merge_EmptyCatalog_EmptyResult(void) {
    // All features disabled — no rotatable screens. Rotation still
    // needs to not crash; the ScreenHandler's "bail out after a full
    // pass" guard handles the runtime side.
    std::vector<int> stored = {3, 0};
    std::vector<int> catalog = {};
    auto result = mergeScreenOrder(stored, catalog);
    TEST_ASSERT_EQUAL_size_t(0, result.size());
}

void test_Merge_FeatureDisabled_IdDropped(void) {
    // Simulates: user stored order includes Bitaxe screens but Bitaxe is
    // currently disabled, so those IDs are absent from this build's
    // catalog. They must drop without affecting the rest of the order,
    // and re-enabling Bitaxe later (which puts them back in the catalog)
    // must restore them — see test_Merge_NewIdAppended for that path.
    std::vector<int> stored = {0, 80, 81, 3};
    std::vector<int> catalog = {0, 3};  // no 80/81 in catalog
    auto result = mergeScreenOrder(stored, catalog);
    TEST_ASSERT_EQUAL_size_t(2, result.size());
    TEST_ASSERT_EQUAL_INT(0, result[0]);
    TEST_ASSERT_EQUAL_INT(3, result[1]);
}

// ---------------------------------------------------------------------------

int runUnityTests(void) {
    UNITY_BEGIN();
    RUN_TEST(test_Parse_Happy);
    RUN_TEST(test_Parse_Empty);
    RUN_TEST(test_Parse_Whitespace_Trimmed);
    RUN_TEST(test_Parse_TrailingComma_SkippedNotError);
    RUN_TEST(test_Parse_LeadingComma_SkippedNotError);
    RUN_TEST(test_Parse_MalformedToken_Dropped);
    RUN_TEST(test_Parse_OnlyWhitespace_Empty);
    RUN_TEST(test_Serialize_Empty);
    RUN_TEST(test_Serialize_Single);
    RUN_TEST(test_Serialize_RoundTrip);
    RUN_TEST(test_Merge_KnownIds_StoredOrderWins);
    RUN_TEST(test_Merge_UnknownIdDropped);
    RUN_TEST(test_Merge_NewIdAppended);
    RUN_TEST(test_Merge_NewIdsPreserveCatalogOrder);
    RUN_TEST(test_Merge_DuplicateStored_FirstWins);
    RUN_TEST(test_Merge_EmptyStored_FullCatalog);
    RUN_TEST(test_Merge_EmptyCatalog_EmptyResult);
    RUN_TEST(test_Merge_FeatureDisabled_IdDropped);
    return UNITY_END();
}

int main(void) {
    return runUnityTests();
}

extern "C" void app_main() {
    runUnityTests();
}
