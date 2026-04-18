# Build

## Prerequisites

- PlatformIO Core ≥ 6.x. The easiest way is `pipx install platformio` or
  the PlatformIO VS Code / Cursor extension, which ships its own `pio`
  under `~/.platformio/penv/bin/`.
- Python ≥ 3.9 (used by the pre- and post-build scripts in `scripts/`).
- Git, and a clone that also pulls the `data/` submodule:

  ```bash
  git clone --recurse-submodules https://git.btclock.dev/btclock/btclock_v3.git
  cd btclock_v3
  ```

  If you cloned without `--recurse-submodules`:

  ```bash
  git submodule update --init --recursive
  ```

The `data/` submodule is the SvelteKit WebUI. It is built separately and
its output is consumed by the firmware via `data_dir = data/build_gz` in
`platformio.ini`.

## Hardware variants

The project targets three physical devices. The firmware auto-selects
the right driver for each via `-D` compile flags set in `platformio.ini`.

| Variant             | Board            | RAM      | PSRAM | Flash | Screens | Frontlight | Flag            |
| ------------------- | ---------------- | -------- | ----- | ----- | ------- | ---------- | --------------- |
| Lolin S3 Mini (v1) | `lolin_s3_mini`  | 320 KB   | 2 MB  | 4 MB  | 7       | no         | `IS_HW_REV_A`   |
| BTClock Rev B       | `btclock_rev_b`  | 320 KB   | 8 MB  | 8 MB  | 7       | yes (PCA9685 + BH1750) | `IS_HW_REV_B` |
| BTClock V8 (proto) | `btclock_v8`     | 320 KB   | 8 MB  | 16 MB | 8       | no         | *(none)*        |

The V8 board routes the e-paper CS / BUSY / RESET pins through two
MCP23017 expanders instead of native GPIO. That means I2C contention
on `mcpMutex` is more expensive there; see
[ARCHITECTURE.md#v8-caveat](ARCHITECTURE.md#v8-caveat-8-panel-prototype).

## Environments

Every shipping variant has a PlatformIO environment; the display size
(213 = 2.13", 29 = 2.9") is part of the env name.

| Env                    | Hardware       | EPD size | Default |
| ---------------------- | -------------- | -------- | ------- |
| `lolin_s3_mini_213epd` | Lolin S3 Mini  | 2.13"    | yes     |
| `lolin_s3_mini_29epd`  | Lolin S3 Mini  | 2.9"     | yes     |
| `btclock_rev_b_213epd` | BTClock Rev B  | 2.13"    | yes     |
| `btclock_rev_b_29epd`  | BTClock Rev B  | 2.9"     | no      |
| `btclock_v8_213epd`    | BTClock V8     | 2.13"    | yes     |
| `native_test_only`     | host           | n/a      | no      |

`platformio.ini`'s `default_envs` covers the four shipping targets, so
`pio run` without `-e` builds all four. `native_test_only` is excluded
from the default build list — it's only picked up by `pio test`
([TESTING.md](TESTING.md)).

All environments share a single `[btclock_base]` block that pins the
espressif32 platform version, the `arduino, espidf` frameworks, the
littlefs filesystem, and a single copy of `platform_packages` and
`lib_deps`. Before 3.4.0 most of that was duplicated per env and a few
versions had drifted; if you need to bump any shared dependency, bump
it in `[btclock_base]` and nowhere else.

## Building

```bash
pio run                          # all four default envs
pio run -e lolin_s3_mini_213epd  # just one
pio run -t clean                 # clean all default envs
pio run -e btclock_rev_b_213epd -t upload  # flash over USB
pio run -e btclock_rev_b_213epd -t uploadfs # flash LittleFS (WebUI)
```

If `pio` isn't on your `$PATH` because you installed it via the IDE:

```bash
export PATH="$HOME/.platformio/penv/bin:$PATH"
```

## Partition layout

Each hardware variant has its own partition table to match its flash
size. All three share the same OTA-slot layout (two equal app slots +
one LittleFS) so the OTA path is the same regardless of target.

| Variant        | Partition file       | OTA slot size | LittleFS (WebUI) |
| -------------- | -------------------- | ------------- | ---------------- |
| Lolin S3 Mini  | `partition.csv`      | 1.75 MB       | 411 KB           |
| BTClock Rev B  | `partition_8mb.csv`  | 3.44 MB       | 820 KB           |
| BTClock V8     | `partition_16mb.csv` | 6.94 MB       | 2 MB             |

When firmware size starts creeping, the Lolin 4 MB variant is the one
that hits the ceiling first — CI pins a size guard against
`lolin_s3_mini_213epd` precisely for this reason
([TESTING.md#firmware-size-guard](TESTING.md#firmware-size-guard)).

## Uploading the WebUI

The WebUI lives in the `data/` submodule and is built separately. The
top-level `data_dir = data/build_gz` in `platformio.ini` means the
LittleFS image PlatformIO packages is whatever is in that directory at
build time.

```bash
cd data
npm ci
npm run build            # emits into build_gz/
cd ..
pio run -e lolin_s3_mini_213epd -t uploadfs
```

The in-place "update WebUI from Git release" button in the device UI
does the same thing over HTTP using the `/api/ota/webui` endpoint
([API.md](API.md)).

## OTA

Two OTA transports are supported:

- **Web-driven streamed download** via `/api/ota/update` and
  `/api/ota/webui`. The handler streams the HTTP response straight into
  `Update.write()` and computes the SHA-256 on the fly, so the whole
  firmware image never has to fit in heap. This is what the WebUI "Check
  for update" flow uses.
- **ArduinoOTA push** on the standard mDNS port. When the NVS key
  `otaPass` is non-empty, it is applied via `ArduinoOTA.setPassword()`
  before `ArduinoOTA.begin()`. Push upload from `pio run -t upload` over
  the network requires you to supply this password to `espota.py`.

## Build-time configuration

Almost every defaulted setting lives in
[`src/lib/system/defaults.hpp`](../src/lib/system/defaults.hpp) as a
`DEFAULT_*` constant. NVS key names live in
[`src/lib/system/pref_keys.hpp`](../src/lib/system/pref_keys.hpp) as
`PrefKeys::…` constants. Don't inline string literals for either — see
[PREFERENCES.md](PREFERENCES.md).

### Third-party library patches

Some third-party Arduino libraries hard-code values that need to vary
across our targets. Rather than fork them, [`scripts/pre_script.py`](../scripts/pre_script.py)
patches the affected header in-place at build time. Each patch is
idempotent via a sentinel comment, so it's safe against `pio run -t
clean` and library reinstalls.

- **`WebSockets.h`** (Links2004 WebSockets library) — wraps
  `#define WEBSOCKETS_MAX_DATA_SIZE (15 * 1024)` in an `#ifndef` guard
  so the `-D WEBSOCKETS_MAX_DATA_SIZE=32768` build flag in
  `[btclock_base]` actually wins. Needed because `mempool.space` pushes
  an ~18 KB initial `blocks` history burst right after our subscription;
  the upstream 15 KB cap would otherwise issue close code 1009
  ("message too big") and park the WS in a reconnect loop.

If you add a new patch, keep it behind a sentinel comment, log "patched
…" when the patch actually applies, and document the upstream fact
that motivated it.

## Troubleshooting

- **`pio: command not found`.** Prepend `~/.platformio/penv/bin` to
  `$PATH` as shown above.
- **`fatal: no submodule mapping found`.** You cloned without
  `--recurse-submodules`; run `git submodule update --init --recursive`.
- **`Linker error: section ... overflows`.** The firmware no longer
  fits in the 4 MB OTA slot. Either drop a feature flag, or build
  against `btclock_rev_b_*` / `btclock_v8_213epd` which have bigger
  slots. CI will catch this before merge via the firmware-size guard.
- **`Update failed: heap too small`.** Pre-3.4.0 the OTA handler would
  `malloc()` the whole image; the streamed path from 3.4.0 onwards
  removes this failure mode. If you still see it, you are running a
  backport on an older branch.
