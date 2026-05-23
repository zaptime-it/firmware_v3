#!/usr/bin/env bash
# Build one or all BTClock variants via the IDF-as-build flow.
#
# Run from anywhere; cd's to repo root automatically.
#
# Usage:
#   ./scripts/build.sh                  # build all 4 production variants
#   ./scripts/build.sh <variant>        # build one
#   ./scripts/build.sh <variant> flash  # build + flash via idf.py (uses $PORT)
#
# Variants:
#   lolin_s3_mini_213epd  (Rev A, 4 MB, 2.13" EPD)
#   lolin_s3_mini_29epd   (Rev A, 4 MB, 2.9" EPD)
#   btclock_rev_b_213epd  (Rev B, 8 MB, 2.13" EPD, frontlight + LDR)
#   btclock_v8_213epd     (V8,  16 MB, 2.13" EPD, 8 panels, octal PSRAM)

set -euo pipefail

# Repo root = directory containing this script's parent dir (scripts/).
cd "$(dirname "$0")/.."

ALL_VARIANTS=(
    lolin_s3_mini_213epd
    lolin_s3_mini_29epd
    btclock_rev_b_213epd
    btclock_v8_213epd
)

build_one() {
    local variant="$1"
    local action="${2:-build}"
    local build_dir=".builds/${variant}"

    echo
    echo "==> $variant ($action)"
    echo

    mkdir -p .builds

    # Drop the stale per-build sdkconfig so the chained defaults take
    # effect even if a previous run for another variant left one behind.
    rm -f "${build_dir}/sdkconfig" sdkconfig

    local args=(
        -B "${build_dir}"
        -DSDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.${variant}"
        -DBTCLOCK_VARIANT="${variant}"
        -DIDF_TARGET=esp32s3
    )
    if [[ "$action" == "flash" ]]; then
        idf.py "${args[@]}" -p "${PORT:?set PORT=/dev/cu.usbmodemXXX}" flash
    else
        idf.py "${args[@]}" build
    fi
}

if [[ $# -eq 0 ]]; then
    for v in "${ALL_VARIANTS[@]}"; do
        build_one "$v"
    done
    echo
    echo "All variants built. Binaries under .builds/<variant>/btclock_v3.bin"
else
    build_one "$1" "${2:-build}"
fi
