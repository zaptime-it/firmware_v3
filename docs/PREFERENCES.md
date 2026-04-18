# Preferences (NVS) inventory

The firmware persists runtime state in the ESP32 NVS (non-volatile
storage) partition via the Arduino `Preferences` wrapper. In 3.4.0 the
string keys are no longer scattered across the codebase — they live in
one place so they can be validated, renamed, and grepped from a single
source.

## The one rule

> **All NVS key names are `PrefKeys::…` constants from
> [`src/lib/system/pref_keys.hpp`](../src/lib/system/pref_keys.hpp).**
> Never inline a `"stringLiteral"` key in firmware code.

The unit test [`test/test_pref_keys/test_main.cpp`](../test/test_pref_keys/test_main.cpp)
enforces three properties on every `PrefKeys::…` constant:

1. **Length cap.** Every key is ≤ 15 characters. The ESP-IDF NVS library
   silently truncates longer keys, which turns two logically-distinct
   keys into the same physical slot.
2. **Non-empty.** Zero-length keys write into the wrong slot.
3. **Uniqueness.** No two constants map to the same key string.

A fourth guard test asserts that the set of keys compiled into the test
matches the count in the header. If you add a new `inline constexpr`
line to `pref_keys.hpp`, add the constant to the test's `kAllKeys`
array too — CI will fail otherwise.

Why a test and not a check at build time? Because the constants are
plain `const char*`, the C++ type system can't express "all members of
namespace X". The test walks them explicitly.

## Defaults

Default values live next to the key they default, but in a separate
header:
[`src/lib/system/defaults.hpp`](../src/lib/system/defaults.hpp) as
`DEFAULT_*` constants. `settings.cpp` uses them in every
`preferences.getString(PrefKeys::Foo, DEFAULT_FOO)` call so the WebUI
sees a populated form on a brand-new device.

When you add a new setting, add:

1. A `PrefKeys::…` constant in `pref_keys.hpp`.
2. A `DEFAULT_…` constant in `defaults.hpp` (if it has a default).
3. A line in `onApiSettingsGet()` in `net/webserver/settings.cpp`
   that reads it.
4. A branch in `onApiSettingsPatch()` that writes it (if the user is
   allowed to change it).
5. A bump to the expected count in `test/test_pref_keys/test_main.cpp`.
6. A matching field in `data/static/swagger.yml` and the WebUI types.

## WebUI compatibility contract

The JSON key names returned by `GET /api/settings` (and accepted by
`PATCH /api/settings`) are *the same strings* as the NVS keys. This is
intentional — it means the WebUI's form state and the device's NVS
record are the same shape, and we don't have to maintain a mapping
table that can drift.

The cost is that renaming an NVS key is a coordinated change across
firmware + WebUI + swagger. Whenever possible, add a new key instead of
renaming an old one. The `customEndpoint` → `ceEndpoint` rename in
3.4.0 is a deliberate breaking change and has no migration shim; the
legacy key is simply gone.

## Secrets

Two keys hold credentials: `httpAuthPass` and `otaPass`. Neither is
ever returned by `GET /api/settings`; the response carries
`httpAuthPassSet: bool` and `otaPassSet: bool` booleans instead. Both
can be cleared by `PATCH`-ing an empty string and set by `PATCH`-ing
the desired value. See [API.md#settings](API.md#settings) for the full
shape.

## Legacy keys

One-shot NVS migrations do not exist in 3.4.0. If a key is retired, it
is retired hard:

- The constant is removed from `pref_keys.hpp`.
- Any migration code that reads the old key is removed.
- The WebUI branch stops writing the old key.

Devices that upgrade from an older firmware will therefore see a
"factory default" value for the retired setting on their next boot.
This is considered acceptable because the WebUI surface for each
setting is small and the user can re-enter the value once.

Retired in 3.4.0:

- `customEndpoint` — use `ceEndpoint` instead.
