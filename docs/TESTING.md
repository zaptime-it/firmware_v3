# Testing

The firmware has two distinct test surfaces:

- **Native (host-side) unit tests** that run on Linux / macOS with no
  hardware involved. These are what CI runs.
- **On-device smoke tests** that require flashing a real board over
  USB. These are a documented local workflow only.

## Native unit tests

All native tests live under `tests/test_<name>/test_main.cpp`:

| Suite                | Scope                                                                                                                                                                                          |
| -------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `test_utils`         | Number formatters (`formatNumberWithSuffix`, MOW mode, K/M/B/T/Q suffix), Lightning invoice amount parsing (`getAmountInSatoshis` with m/u/n/p), hash-rate helpers.                            |
| `test_datahandler`   | Screen content composition in `data_handler.hpp`.                                                                                                                                              |
| `test_bitaxehandler` | Bitaxe JSON → screen content mapping.                                                                                                                                                          |
| `test_nostrdisplay`  | Nostr event → screen content mapping.                                                                                                                                                          |
| `test_mining_pool`   | Pool adapters (Ocean, Braiins, Public Pool) — response shape → display.                                                                                                                        |
| `test_dnd_window`    | Pure DND time-range algebra (`lib/btclock/dnd_window.hpp`). Same-day windows, midnight wrap, one-minute windows, whole-day-minus-one windows, `start == end` degenerate case. Added in 3.4.0.  |
| `test_pref_keys`     | NVS key inventory guard rails: 15-char cap, non-empty, uniqueness, and a guard that the test table matches the header count. Added in 3.4.0.                                                   |
| `test_price_policy`  | `price_policy.hpp` decision logic for currency conversion / per-screen formatting.                                                                                                             |
| `test_data_source_policy` | Gate that suppresses the "data source disconnected" LED effect while WiFi itself is down (to avoid a long purple-red strobe).                                                              |
| `test_screen_nav`    | Screen / currency navigation helpers in `lib/btclock/screen_nav.hpp`.                                                                                                                          |
| `test_screen_order`  | User-configurable screen rotation order, including the catalog-merge logic that drops disabled IDs and reserves the bitaxe icon / pool label slots.                                            |

These suites link the code under `lib/btclock/` (plus, for
`test_pref_keys`, the `PrefKeys::` constants from
`src/lib/system/pref_keys.hpp`) against
[Unity](https://www.throwtheswitch.org/unity) and run on the host. No
Arduino, FreeRTOS, WiFi, or MCP stack is involved, so the whole suite
finishes in ≲ 2 s.

### Running the native tests

The test harness is a stock CMake project under `tests/`. Configure
and build once, then invoke `ctest` for fast re-runs.

```bash
cmake -G Ninja -B build-tests -S tests
cmake --build build-tests -j
ctest --test-dir build-tests --output-on-failure
```

Filter to one suite:

```bash
ctest --test-dir build-tests -R test_dnd_window --output-on-failure
```

For the sanitizer build (ASan + UBSan, matches `BTCLOCK_TEST_SANITIZE=ON`):

```bash
cmake -G Ninja -B build-tests-asan -S tests -DBTCLOCK_TEST_SANITIZE=ON
cmake --build build-tests-asan -j
ASAN_OPTIONS=detect_leaks=1:abort_on_error=1 \
UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1 \
ctest --test-dir build-tests-asan --output-on-failure
```

Unity itself is fetched at configure time via `FetchContent` (pinned
to v2.6.0 in `tests/CMakeLists.txt`); no vendored copy.

### Adding a test

1. Create `tests/test_<subject>/test_main.cpp`.
2. Include the header you want to exercise. If the header transitively
   pulls in Arduino / FreeRTOS / ESP-IDF, it can't be unit-tested on
   the host; extract the pure logic into `lib/btclock/` first (that's
   what [`dnd_window`](../lib/btclock/dnd_window.hpp) does) or add it
   to the on-device test list instead.
3. Write the test with the standard Unity macros
   (`TEST_ASSERT_*`, `RUN_TEST`).
4. Re-run cmake — `tests/CMakeLists.txt` globs `test_*` so no edits
   needed there.
5. Run with `ctest --test-dir build-tests -R test_<subject>`.

If your test file doesn't define `setUp` / `tearDown`, the weak default
stubs from `tests/unity_defaults.c` keep the linker happy.

### DND time window

`test_dnd_window` pins the behaviour of
`btclock::isTimeInDNDRange(h, m, startH, startM, endH, endM)`:

- Same-day window (`22:00 → 23:00`): `22:30` ∈ window.
- Midnight-crossing (`22:00 → 06:00`): `23:00` and `02:30` ∈ window,
  `12:00` ∉.
- Boundary: `start` ∈ window, `end` ∉ window.
- One-minute window: `22:00 → 22:01` contains exactly one minute.
- `start == end` is treated as "DND window disabled" — picking the same
  time in both dropdowns cannot accidentally lock DND on forever. This
  is a behaviour change from earlier firmware versions.

The LedHandler delegates to this pure function, so both the runtime
path and the test path exercise the same algorithm.

### Pref keys

`test_pref_keys` walks every `PrefKeys::…` constant and asserts:

1. `strlen(key) ≤ 15` (the ESP-IDF NVS name cap).
2. `strlen(key) > 0`.
3. No two constants share the same value.
4. The test's local copy of the keys (`kAllKeys[]`) contains exactly
   the number the header defines. If you add a new `inline constexpr`
   key to `pref_keys.hpp`, you must also add it to `kAllKeys[]` and
   bump the expected count — CI catches drift loudly otherwise.

See [PREFERENCES.md](PREFERENCES.md) for the "why" of those rules.

## On-device tests

There is no automated on-device test harness in CI. The Forgejo CI
container builds firmware but does not flash it — running Unity tests
against a real MCP23017 + GxEPD2 stack requires a USB-attached board
and lives outside the automated flow.

For local on-device smoke tests of the assembled board, flash the
release image and prod the WebUI / API by hand:

```bash
PORT=/dev/cu.usbmodemXXXX ./firmware/build.sh lolin_s3_mini_213epd flash
```

That gets the firmware on the board; the LittleFS image still has to
be flashed separately (the device's `/upload/webui` endpoint is the
easiest path — see [API.md](API.md)).

## CI

### Forgejo (primary)

`.forgejo/workflows/push.yaml` is the canonical CI pipeline. On every
push and tag it:

1. Checks out with submodules.
2. Runs `host-tests`: configures `tests/CMakeLists.txt` in both plain
   and sanitize mode, builds, runs `ctest --output-on-failure`.
3. `build` matrix runs `firmware/build-release.sh <variant>` per
   variant. PRs build only `lolin_s3_mini_213epd` + `btclock_rev_b_213epd`
   to keep PR latency reasonable; tags + workflow_dispatch build the
   full 4-variant matrix.
4. `release` (tag pushes only): pulls every per-variant artifact,
   stages the release directory + `manifest.json`, computes per-asset
   sha256, publishes the release through the Forgejo release action,
   and pushes the bundle to the `btclock/web-flasher` repo.

### GitHub (mirror)

`.github/workflows/` holds a reduced mirror. It tracks the Forgejo
pipeline — if you add a step to one CI, mirror it to the other.

### Lint

`.forgejo/workflows/lint.yaml` runs clang-format and clang-tidy. The
tidy job derives `compile_commands.json` from the host-test CMake
project (`CMAKE_EXPORT_COMPILE_COMMANDS=ON` is on by default in
`tests/CMakeLists.txt`) — that's a clean compilation set with no
ESP32-only headers leaking in.

## Pitfalls

- **Don't `#include <Arduino.h>` in a native test.** The host has no
  Arduino toolchain. If your target pulls in Arduino, extract the
  pure logic into `lib/btclock/` first.
- **Don't `strlen()` a possibly-null `const char*` on macOS.** It
  spins forever instead of crashing. Always null-check. (The bug that
  surfaced this was an off-by-one in the `test_pref_keys` array size.)
- **Re-running cmake doesn't pick up a deleted source file**
  automatically — if a test file disappears, cmake still has a stale
  entry. `rm -rf build-tests` and reconfigure when in doubt.
