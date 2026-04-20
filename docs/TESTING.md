# Testing

The firmware has two distinct test surfaces:

- **Native (host-side) unit tests** that run on Linux / macOS with no
  hardware involved. These are what CI runs.
- **On-device smoke tests** that require flashing a real board over
  USB. These are a documented local workflow only.

## Native unit tests

All native tests live under `test/` in the standard PlatformIO layout —
one subdirectory per suite, each with a `test_main.cpp`:

| Suite               | Scope                                                                |
| ------------------- | -------------------------------------------------------------------- |
| `test_utils`        | Number formatters (`formatNumberWithSuffix`, MOW mode, K/M/B/T/Q suffix), Lightning invoice amount parsing (`getAmountInSatoshis` with m/u/n/p), hash-rate helpers. |
| `test_datahandler`  | Screen content composition in `data_handler.hpp`.                    |
| `test_bitaxehandler`| Bitaxe JSON → screen content mapping.                                |
| `test_nostrdisplay` | Nostr event → screen content mapping.                                |
| `test_mining_pool`  | Pool adapters (Ocean, Braiins, Public Pool) — response shape → display. |
| `test_dnd_window`   | Pure DND time-range algebra (`lib/btclock/dnd_window.hpp`). Same-day windows, midnight wrap, one-minute windows, whole-day-minus-one windows, `start == end` degenerate case. Added in 3.4.0. |
| `test_pref_keys`    | NVS key inventory guard rails: 15-char cap, non-empty, uniqueness, and a guard that the test table matches the header count. Added in 3.4.0. |

These suites link the code under `lib/btclock/` (plus in `test_pref_keys`'s
case, the `PrefKeys::` constants from `src/lib/system/pref_keys.hpp`)
against the [Unity](https://www.throwtheswitch.org/unity) test framework
and run on the host. No Arduino, FreeRTOS, WiFi, or MCP stack is
involved, so tests finish in ≲ 6 s for the whole suite.

### Running the native tests

```bash
export PATH="$HOME/.platformio/penv/bin:$PATH"   # if pio isn't on PATH
pio test -e native_test_only
```

Filter to one suite:

```bash
pio test -e native_test_only -f test_dnd_window
```

### Adding a test

1. Create `test/test_<subject>/test_main.cpp`.
2. Include the header you want to exercise. If the header transitively
   pulls in Arduino / FreeRTOS / ESP-IDF, it can't be unit-tested on
   the host; either extract the pure logic into `lib/btclock/` first
   (that's what [`dnd_window`](../lib/btclock/dnd_window.hpp) does) or
   add it to the on-device test list instead.
3. Write the test with the standard Unity macros
   (`TEST_ASSERT_*`, `RUN_TEST`).
4. Run it via `pio test -e native_test_only -f test_<subject>`.

### DND time window

`test_dnd_window` pins the behaviour of
`btclock::isTimeInDNDRange(h, m, startH, startM, endH, endM)`:

- Same-day window (`22:00 → 23:00`): `22:30` ∈ window.
- Midnight-crossing (`22:00 → 06:00`): `23:00` and `02:30` ∈ window, `12:00` ∉.
- Boundary: `start` ∈ window, `end` ∉ window.
- One-minute window: `22:00 → 22:01` contains exactly one minute.
- `start == end` is treated as "DND window disabled" — picking the same
  time in both dropdowns cannot accidentally lock DND on forever. This
  is a behaviour change from earlier firmware versions.

The LedHandler delegates to this pure function, so both the runtime path
and the test path exercise the same algorithm.

### Pref keys

`test_pref_keys` walks every `PrefKeys::…` constant and asserts:

1. `strlen(key) ≤ 15` (the ESP-IDF NVS name cap).
2. `strlen(key) > 0`.
3. No two constants share the same value.
4. The test's local copy of the keys (`kAllKeys[]`) contains exactly
   the number the header defines. If you add a new `inline constexpr`
   key to `pref_keys.hpp`, you must also add it to `kAllKeys[]` and
   bump the expected count — CI will catch it loudly otherwise, rather
   than silently skipping half the inventory.

See [PREFERENCES.md](PREFERENCES.md) for the "why" of those rules.

## On-device tests

There is no automated on-device test harness in CI. The Forgejo CI
container builds firmware but does not flash it to hardware; running
Unity tests against a real MCP23017 + GxEPD2 stack requires a USB-
attached board and lives in a commented HIL (hardware-in-the-loop) hook
in the Forgejo workflow for future use.

For local on-device smoke tests, the standard PlatformIO flow is:

```bash
pio test -e lolin_s3_mini_213epd --upload-port /dev/ttyACM0
```

This builds a per-suite firmware image, flashes it, and reads the
Unity output over serial. It is intentionally a local, human-driven
workflow — the only tests that make sense to run this way are ones
that actually exercise the ESP32 peripherals (MCP I/O, BH1750 light
reads, PCA9685 PWM), and those are usually one-off debugging aids
rather than regression-worthy tests.

## CI

### Forgejo (primary)

`.forgejo/workflows/push.yaml` is the canonical CI pipeline. On every
push and tag it:

1. Checks out with submodules.
2. Installs pnpm + node for the WebUI and pip + PlatformIO for
   firmware.
3. Runs `pio test -e native_test_only` and emits JUnit XML into
   `junit-reports/`.
4. Builds all four default PlatformIO envs (`pio run`).
5. Builds the LittleFS filesystem image (`pio run --target buildfs`).
6. Merges bootloader + partitions + firmware + littlefs into a
   single flashable binary per hardware variant.
7. Computes SHA-256 sums for the merged binary, firmware binary, and
   filesystem partition.
8. Uploads artifacts; on a tag, publishes a release with the binaries
   and checksums attached.

### GitHub (mirror)

`.github/workflows/` holds a reduced mirror. It intentionally tracks
Forgejo and should not acquire its own CI steps — if you add a step
to one CI, mirror it to the other. The device autoupdate endpoint
only fetches from the Forgejo release, so GitHub's job is to keep the
mirror usable as a backup.

### Firmware-size guard (planned)

The Lolin S3 Mini has a 1.75 MB OTA slot
([BUILD.md#partition-layout](BUILD.md#partition-layout)) and is the
first target that will refuse to boot if the firmware overflows.
Adding a `pio check` step that fails the CI job if
`.pio/build/lolin_s3_mini_213epd/firmware.bin` exceeds the slot size
(with a small headroom margin) is tracked as a low-priority
follow-up; until then, CI will still fail at link time when this
happens, just with a less friendly message than a dedicated check.

## Pitfalls

- **Don't `#include <Arduino.h>` in a native test.** The `native`
  platform has no Arduino toolchain. If your target pulls in Arduino,
  extract the pure logic into `lib/btclock/` first.
- **Don't `strlen()` a possibly-null `const char*` on macOS.** It
  spins forever instead of crashing. Always null-check. (The bug that
  surfaced this was an off-by-one in the `test_pref_keys` array size.)
- **Run `pio test` from the repo root**, not from inside a test
  subdirectory — PlatformIO uses `platformio.ini` to find the test env.
- **After a `pio test` crash, kill stragglers.** PlatformIO spawns the
  test binary as a subprocess and a hang can leave zombies:
  `pkill -9 -f "native_test_only/program"`.
