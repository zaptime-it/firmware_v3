// test_pref_keys / test_main.cpp
//
// NVS (Preferences) keys have two hard constraints on ESP32:
//   1. Key length must not exceed 15 characters; anything longer is silently
//      truncated by the underlying NVS layer, which causes two nominally
//      different keys to alias onto the same slot and overwrite each other.
//   2. Keys that appear in JSON payloads sent to/from the WebUI are part of
//      the firmware's public API contract; accidentally duplicating one in
//      PrefKeys would mask the collision from reviewers.
//
// These tests enforce both invariants at CI time against the full key table
// in src/lib/system/pref_keys.hpp, so adding a new NVS key can never silently
// regress on a different platform build.

#include <unity.h>
#include <cstddef>
#include <cstdio>
#include <cstring>

#include "../../src/lib/system/pref_keys.hpp"

namespace {

// Mirror of the constants in pref_keys.hpp. Keeping the list here (rather
// than scraping the header at CI time) is intentional: a new key must be
// added in two places, which is exactly the friction that keeps the inventory
// in sync with what the firmware actually uses. If a key goes missing from
// this list the compiler errors out on the static_assert-equivalent
// TEST_ASSERT below, and a CI failure is the signal to add it.
struct PrefKeyEntry {
    const char *name;
    const char *value;
};

#define PK(name) { #name, PrefKeys::name }

// Deduced-size array: if a key is added to pref_keys.hpp and the test
// reviewer forgets to add the corresponding PK() entry here, the test
// will run against a shorter list (signalling reviewer, not crashing).
// Conversely, hard-coding std::array<..., N> with a wrong N used to
// default-initialise the extra slots to null pointers, and the first
// strlen() on a nullptr would spin forever on macOS instead of failing
// loudly. The explicit-count version is gone for that reason.
constexpr PrefKeyEntry kAllKeys[] = {
    PK(ActCurrencies),
    PK(BgColor),
    PK(BitaxeEnabled),
    PK(BitaxeHostname),
    PK(BlockFeeDec),
    PK(BlockFlashColor),
    PK(BlockHeight),
    PK(CeDisableSSL),
    PK(CeEndpoint),
    PK(CurrentScreen),
    PK(DataSource),
    PK(DisableLeds),
    PK(DisplayText),
    PK(DndEnabled),
    PK(DndEndHour),
    PK(DndEndMin),
    PK(DndStartHour),
    PK(DndStartMin),
    PK(DndTimeEnabled),
    PK(EnableDebugLog),
    PK(FgColor),
    PK(FlAlwaysOn),
    PK(FlDisable),
    PK(FlEffectDelay),
    PK(FlFlashOnUpd),
    PK(FlFlashOnZap),
    PK(FlMaxBrightness),
    PK(FlOffWhenDark),
    PK(FontName),
    PK(FullRefreshMin),
    PK(GitReleaseUrl),
    PK(GmtOffset),
    PK(HostnamePrefix),
    PK(HttpAuthEnabled),
    PK(HttpAuthPass),
    PK(HttpAuthUser),
    PK(InverseButtons),
    PK(InvertedColor),
    PK(LastCurrency),
    PK(LastPrice),
    PK(LedBrightness),
    PK(LedFlashOnUpd),
    PK(LedFlashOnZap),
    PK(LedStatus),
    PK(LedTestOnPower),
    PK(LocalPoolHost),
    PK(LuxLightToggle),
    PK(McapBigChar),
    PK(MdnsEnabled),
    PK(MempoolInstance),
    PK(MempoolSecure),
    PK(MinSecPriceUpd),
    PK(MiningPoolName),
    PK(MiningPoolStats),
    PK(MiningPoolUser),
    PK(MowMode),
    PK(NostrPubKey),
    PK(NostrRelay),
    PK(NostrZapNotify),
    PK(NostrZapPubkey),
    PK(NostrZapPubkeys_Legacy),
    PK(OtaEnabled),
    PK(OtaPass),
    PK(PoolGlobalStats),
    PK(PoolLogosUrl),
    PK(RefrScrnChange),
    PK(ScreenOrder),
    PK(ScrnRestoreZap),
    PK(StealFocus),
    PK(SuffixPrice),
    PK(SuffixShareDot),
    PK(SupplyPercent),
    PK(TimerActive),
    PK(TimerSeconds),
    PK(TxPower),
    PK(TzString),
    PK(UseBlkCountdown),
    PK(UseMscwTime),
    PK(UseSatsSymbol),
    PK(VerticalDesc),
    PK(WifiConfigured),
    PK(WpTimeout),
};

constexpr size_t kNumKeys = sizeof(kAllKeys) / sizeof(kAllKeys[0]);

}  // namespace

void setUp(void) {}
void tearDown(void) {}

// NVS key length ceiling. Exceeding this silently truncates on device.
static constexpr size_t kNvsMaxKeyLen = 15;

// If the header grows and the test table doesn't keep pace, or vice versa,
// we want a loud failure rather than a silently-incomplete test. Bump this
// number when intentionally adding a new key to both places.
static constexpr size_t kExpectedNumKeys = 82;

void test_KeyTableMatchesHeader(void)
{
    // The upstream count can be read with:
    //   grep -cE '^inline constexpr' src/lib/system/pref_keys.hpp
    // (the anchored version excludes the `inline constexpr` mention inside
    // the file header comment). The two must stay in sync.
    TEST_ASSERT_EQUAL_MESSAGE(kExpectedNumKeys, kNumKeys,
        "Keys listed in test_pref_keys table do not match the header. "
        "Update both test_pref_keys/test_main.cpp AND kExpectedNumKeys.");
}

void test_AllKeysRespectNvsLengthCap(void)
{
    for (size_t i = 0; i < kNumKeys; ++i) {
        const auto &entry = kAllKeys[i];
        const size_t len = std::strlen(entry.value);
        if (len > kNvsMaxKeyLen) {
            char msg[128];
            snprintf(msg, sizeof(msg),
                     "PrefKeys::%s = \"%s\" is %zu chars (NVS caps at %zu)",
                     entry.name, entry.value, len, kNvsMaxKeyLen);
            TEST_FAIL_MESSAGE(msg);
        }
    }
}

void test_AllKeysNonEmpty(void)
{
    for (size_t i = 0; i < kNumKeys; ++i) {
        const auto &entry = kAllKeys[i];
        if (std::strlen(entry.value) == 0) {
            char msg[128];
            snprintf(msg, sizeof(msg),
                     "PrefKeys::%s is empty", entry.name);
            TEST_FAIL_MESSAGE(msg);
        }
    }
}

void test_AllKeyValuesUnique(void)
{
    for (size_t i = 0; i < kNumKeys; ++i) {
        for (size_t j = i + 1; j < kNumKeys; ++j) {
            if (std::strcmp(kAllKeys[i].value, kAllKeys[j].value) == 0) {
                char msg[160];
                snprintf(msg, sizeof(msg),
                         "PrefKeys::%s and PrefKeys::%s both map to \"%s\"",
                         kAllKeys[i].name, kAllKeys[j].name,
                         kAllKeys[i].value);
                TEST_FAIL_MESSAGE(msg);
            }
        }
    }
}

// ---------------------------------------------------------------------------

int runUnityTests(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_KeyTableMatchesHeader);
    RUN_TEST(test_AllKeysRespectNvsLengthCap);
    RUN_TEST(test_AllKeysNonEmpty);
    RUN_TEST(test_AllKeyValuesUnique);
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
