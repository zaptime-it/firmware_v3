# Build

## Prerequisites

- **ESP-IDF v5.5** with the esp32s3 toolchain. Install via:

  ```bash
  git clone --depth 1 --branch v5.5 --recurse-submodules \
    --shallow-submodules https://github.com/espressif/esp-idf.git ~/esp/esp-idf
  ~/esp/esp-idf/install.sh esp32s3
  source ~/esp/esp-idf/export.sh
  ```

  Once installed, run `source ~/esp/esp-idf/export.sh` in every fresh
  shell that needs `idf.py`.

- **Node + pnpm** (for the WebUI build).
- **Python 3** (already pulled in by ESP-IDF) plus
  `littlefs-python==0.15.0` and `esptool` on the IDF venv:

  ```bash
  pip install --upgrade littlefs-python==0.15.0 esptool
  ```

- **Git** with submodules, so the `data/` WebUI submodule comes along:

  ```bash
  git clone --recurse-submodules https://git.btclock.dev/btclock/btclock_v3.git
  cd btclock_v3
  # or, after a non-recursive clone:
  git submodule update --init --recursive
  ```

The `data/` submodule is the SvelteKit WebUI. It builds to
`data/build_gz/www/` (gzipped per file) and is packed into a LittleFS
image at build-release time.

## Hardware variants

The project targets three physical devices. The firmware auto-selects
the right driver via `-D` compile flags from
`main/CMakeLists.txt`, keyed off the `BTCLOCK_VARIANT` CMake
variable.

| Variant            | Board            | RAM    | PSRAM | Flash | Screens | Frontlight             | Flag            |
| ------------------ | ---------------- | ------ | ----- | ----- | ------- | ---------------------- | --------------- |
| Lolin S3 Mini (v1) | `lolin_s3_mini`  | 320 KB | 2 MB  | 4 MB  | 7       | no                     | `IS_HW_REV_A`   |
| BTClock Rev B      | `btclock_rev_b`  | 320 KB | 8 MB  | 8 MB  | 7       | yes (PCA9685 + BH1750) | `IS_HW_REV_B`   |
| BTClock V8 (proto) | `btclock_v8`     | 320 KB | 8 MB  | 16 MB | 8       | no                     | `IS_BTCLOCK_V8` |

The V8 board routes the e-paper CS / BUSY / RESET pins through two
MCP23017 expanders instead of native GPIO. That means I2C contention
on `mcpMutex` is more expensive there; see
[ARCHITECTURE.md#v8-caveat](ARCHITECTURE.md#v8-caveat-8-panel-prototype).

## Variants

Each shipping variant has its own `sdkconfig.defaults.<variant>`
plus a `BTCLOCK_VARIANT` selector that picks the right pin defines:

| Variant                | Hardware       | EPD size | CI default |
| ---------------------- | -------------- | -------- | ---------- |
| `lolin_s3_mini_213epd` | Lolin S3 Mini  | 2.13"    | yes        |
| `lolin_s3_mini_29epd`  | Lolin S3 Mini  | 2.9"     | yes        |
| `btclock_rev_b_213epd` | BTClock Rev B  | 2.13"    | yes        |
| `btclock_v8_213epd`    | BTClock V8     | 2.13"    | yes        |

A shared `sdkconfig.defaults` carries the cross-variant size
trim (Mozilla CMN cert bundle, mbedtls / WiFi / Arduino-selective trims,
assertions silent, etc.). Per-variant defaults chain on top of it via
the `SDKCONFIG_DEFAULTS` argument the build helpers pass.

## Building

The wrapper scripts in `scripts/` are the canonical entry point.
They cd to the repo root, drop the stale per-build `sdkconfig`
checkpoint so the chained defaults take effect, and run `idf.py` with
the right arguments.

```bash
# Build every shipping variant in turn; binaries land in build_<variant>/
./scripts/build.sh

# Build one variant
./scripts/build.sh lolin_s3_mini_213epd

# Build + flash (PORT must point at a board in download mode)
PORT=/dev/cu.usbmodem8331401 ./scripts/build.sh btclock_rev_b_213epd flash

# Build + LittleFS image + esptool merge_bin (release flow, used by CI)
./scripts/build-release.sh btclock_v8_213epd
# Artifacts land in release-stage/<variant>/.
```

If you want to call `idf.py` directly (from repo root):

```bash
source ~/esp/esp-idf/export.sh
# One-time, idempotent: vendor the Arduino libraries listed in
# arduino_libraries.json into arduino_libraries/<name>/.
python3 scripts/fetch_arduino_libs.py
idf.py -B build_lolin_s3_mini_213epd \
       -DSDKCONFIG_DEFAULTS='sdkconfig.defaults;sdkconfig.defaults.lolin_s3_mini_213epd' \
       -DBTCLOCK_VARIANT=lolin_s3_mini_213epd \
       -DIDF_TARGET=esp32s3 \
       build
```

## Partition layout

Each hardware variant has its own partition table to match its flash
size. All three share the same OTA-slot layout (two equal app slots
plus one LittleFS for the WebUI) so the OTA path is the same regardless
of target.

| Variant       | Partition file                  | OTA slot size | LittleFS (WebUI) |
| ------------- | ------------------------------- | ------------- | ---------------- |
| Lolin S3 Mini | `partition.csv`        | 1.72 MB       | 411 KB           |
| BTClock Rev B | `partition_8mb.csv`    | 3.44 MB       | 820 KB           |
| BTClock V8    | `partition_16mb.csv`   | 6.94 MB       | 2 MB             |

The Lolin 4 MB variant is the one that hits the OTA-slot ceiling first
(currently ~3% free on `lolin_s3_mini_213epd`). When firmware size
creeps, that's the one to watch.

## Uploading the WebUI

The WebUI lives in the `data/` submodule and is built separately;
`scripts/build-release.sh` consumes the already-built output.

```bash
cd data
CI=true pnpm install --frozen-lockfile
pnpm build
python3 gzip_build.py          # repacks dist/ into build_gz/www/*.gz
```

After that, either:

- The WebUI "update from release" button on the device pulls the
  matching `littlefs_<size>.bin` over HTTP, or
- Flash `release-stage/<variant>/littlefs_<size>.bin` directly via the
  device's `/upload/webui` endpoint (see [API.md](API.md)) or esptool:

  ```bash
  source ~/esp/esp-idf/export.sh
  python -m esptool --chip esp32s3 -p $PORT write_flash \
    <spiffs_offset> release-stage/<variant>/littlefs_<size>.bin
  ```

`build-release.sh` autodetects the spiffs offset from the built
partition table — don't hardcode `0x380000` from older docs; on Lolin
boards app1 rounds up to a 64 KB boundary so the actual offset is
`0x388000`.

## OTA

Two OTA transports are supported:

- **Web-driven streamed download** via `/api/ota/update` and
  `/api/ota/webui`. The handler streams the HTTP response straight
  into `Update.write()` and computes the SHA-256 on the fly, so the
  whole firmware image never has to fit in heap. This is what the
  WebUI "Check for update" flow uses.
- **ArduinoOTA push** on the standard mDNS port. When the NVS key
  `otaPass` is non-empty, it is applied via `ArduinoOTA.setPassword()`
  before `ArduinoOTA.begin()`.

## Build-time configuration

Almost every defaulted setting lives in
[`main/lib/system/defaults.hpp`](../main/lib/system/defaults.hpp) as a
`DEFAULT_*` constant. NVS key names live in
[`main/lib/system/pref_keys.hpp`](../main/lib/system/pref_keys.hpp) as
`PrefKeys::…` constants. Don't inline string literals for either — see
[PREFERENCES.md](PREFERENCES.md).

### Third-party library patches

Some vendored Arduino libraries hard-code values that need to vary
across our targets. Rather than fork them, `scripts/fetch_arduino_libs.py`
patches the affected files in-place at fetch time. Each patch is
idempotent via a sentinel comment, so re-running the fetcher (or
`./scripts/build.sh`, which calls it transitively the first time)
won't double-apply.

Currently patched:

- **WebSockets.h** (Links2004) — wraps the `WEBSOCKETS_MAX_DATA_SIZE`
  default in an `#ifndef` guard so our `-D WEBSOCKETS_MAX_DATA_SIZE=32768`
  override actually wins. Needed because mempool.space pushes an ~18 KB
  initial `blocks` history burst right after our subscription; the
  upstream 15 KB cap would otherwise issue close code 1009 and park
  the WS in a reconnect loop.
- **WebSockets** RX buffer DRAM placement — keeps the buffer out of
  PSRAM so the WS task doesn't stall on the cross-bus access.
- **GxEPD2** modern-GCC fix-ups (`if (_rst >= 0)` → `if (_rst != nullptr)`
  for the universal_pin fork, plus a `<stdexcept>` include).
- **uBitcoin** `esp_random` shim so the host build can resolve the
  IDF-provided symbol.

If you add a new patch, keep it behind a sentinel comment, log "patched
…" when the patch actually applies, and document the upstream fact
that motivated it.

## Troubleshooting

- **`idf.py: command not found`.** You haven't sourced ESP-IDF in this
  shell. Run `source ~/esp/esp-idf/export.sh`.
- **`Build directory '…' configured for project '…' not '…'`.** A
  stale `build_<variant>/` CMake cache from before a rename. Delete it
  (`rm -rf build_<variant>`) and rerun.
- **`fatal: no submodule mapping found`.** You cloned without
  `--recurse-submodules`; run `git submodule update --init --recursive`.
- **`partition.csv missing`.** You're still on a tree from before the
  PIO removal — the file moved to `partition.csv`. Update
  your branch.
- **`Linker error: section ... overflows`.** The firmware no longer
  fits in the 4 MB OTA slot. Either drop a feature flag, or build
  against `btclock_rev_b_213epd` / `btclock_v8_213epd` which have
  larger slots. CI catches this at the partition-size check before
  the artifact uploads.
- **`Update failed: heap too small`** during web OTA. Pre-3.4.0 the
  OTA handler `malloc()`'d the whole image; the streamed path from
  3.4.0 onwards removes this failure mode. If you still see it, you
  are running a backport on an older branch.
