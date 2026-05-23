#!/usr/bin/env bash
# Build a single BTClock variant end-to-end and stage release-ready
# artifacts under release-stage/<variant>/. Used by both the GitHub and
# Forgejo CI workflows so the per-variant pipeline isn't duplicated in
# YAML across providers.
#
# Inputs:
#   $1 = variant name (lolin_s3_mini_213epd | lolin_s3_mini_29epd
#                     | btclock_rev_b_213epd | btclock_v8_213epd)
#
# Pre-requisites assumed by the caller:
#   - $IDF_PATH is set (idf.py available); we'll source export.sh.
#   - The webui has already been built; data/build_gz/www/ exists
#     (run data/pnpm build && data/python3 gzip_build.py first).
#   - littlefs-python is on the IDF venv's PATH.
#
# Outputs (in repo-root release-stage/<variant>/):
#   firmware.bin              raw OTA app image (PIO-equivalent name)
#   bootloader.bin            second-stage bootloader
#   partitions.bin            partition table
#   ota_data_initial.bin      initial OTA selector (boots app0)
#   littlefs_<size>.bin       webui LittleFS image (size = 4MB/8MB/16MB)
#   <variant>.bin             merged single-flash image
#   *.sha256                  per-artifact sha256

set -euo pipefail

variant="${1:?variant name required}"
repo_root="$(cd "$(dirname "$0")/.." && pwd)"
build_dir="${repo_root}/build_${variant}"
stage_dir="${repo_root}/release-stage/${variant}"
data_root="${repo_root}/data/build_gz/www"

case "$variant" in
    lolin_s3_mini_213epd|lolin_s3_mini_29epd)
        flash_size=4MB
        partition_table="${repo_root}/firmware/partition.csv"
        ;;
    btclock_rev_b_213epd)
        flash_size=8MB
        partition_table="${repo_root}/firmware/partition_8mb.csv"
        ;;
    btclock_v8_213epd)
        flash_size=16MB
        partition_table="${repo_root}/firmware/partition_16mb.csv"
        ;;
    *)
        echo "Unknown variant '$variant'" >&2
        exit 64
        ;;
esac

# ---- 1. Build the firmware ----
( cd "${repo_root}/firmware" && ./build.sh "$variant" )

# ---- 2. Build the LittleFS image ----
# Parse the spiffs partition size from the matching CSV (in case the
# layouts ever drift). Hex sizes are normal in IDF partition tables.
fs_size_hex=$(awk -F',' '
    /^[[:space:]]*spiffs[[:space:]]*,/ {
        # Field 5 is size; trim whitespace.
        gsub(/^[[:space:]]+|[[:space:]]+$/, "", $5)
        print $5
        exit
    }' "$partition_table")
if [[ -z "$fs_size_hex" ]]; then
    echo "Could not find spiffs partition size in $partition_table" >&2
    exit 65
fi

# esp_littlefs at runtime drops any partial trailing block
# (block_count = partition_size / 4096, integer div). Match that here so
# the on-disk block_count in the image equals what runtime expects;
# otherwise lfs_mount fails with LFS_ERR_INVAL. partition_size on
# Lolin (0x66C00) is 102.75 blocks → round down to 102 blocks = 0x66000.
fs_blocks=$(( fs_size_hex / 4096 ))
fs_image_size=$(( fs_blocks * 4096 ))

mkdir -p "$stage_dir"
fs_bin="${stage_dir}/littlefs_${flash_size}.bin"

# CONFIG_LITTLEFS_OBJ_NAME_LEN=64 in sdkconfig.defaults — the image's
# on-disk name_max must match or runtime mount fails.
littlefs-python create "$data_root" "$fs_bin" -v \
    --fs-size="$fs_image_size" \
    --name-max=64 \
    --block-size=4096

# ---- 3. Copy IDF outputs with PIO-equivalent names ----
cp "${build_dir}/btclock_v3.bin"                      "${stage_dir}/firmware.bin"
cp "${build_dir}/bootloader/bootloader.bin"               "${stage_dir}/bootloader.bin"
cp "${build_dir}/partition_table/partition-table.bin"     "${stage_dir}/partitions.bin"
cp "${build_dir}/ota_data_initial.bin"                    "${stage_dir}/ota_data_initial.bin"

# ---- 4. Find the spiffs partition offset for the merge ----
# The offset depends on every preceding partition; compute it the same
# way IDF does instead of hardcoding (Rev A is 0x388000 not 0x380000
# because app1 rounds up to a 64 KB boundary — that bit me once already).
fs_offset_hex=$(python3 - "$build_dir/partition_table/partition-table.bin" <<'PY'
import sys, struct
with open(sys.argv[1], 'rb') as f:
    data = f.read()
# Each partition entry is 32 bytes: magic(2) type(1) subtype(1) offset(4) size(4) name(16) flags(4)
for off in range(0, len(data), 32):
    entry = data[off:off+32]
    if len(entry) < 32 or entry[0:2] != b'\xaa\x50':
        break
    p_offset, p_size = struct.unpack('<II', entry[4:12])
    name = entry[12:28].rstrip(b'\x00').decode()
    if name == 'spiffs':
        print(f'0x{p_offset:x}')
        break
PY
)
if [[ -z "$fs_offset_hex" ]]; then
    echo "Could not find spiffs partition offset in build's partition table" >&2
    exit 66
fi

# ---- 5. Merge into a single-flash image ----
# Same offsets as the legacy PIO recipe in .github/workflows/tagging.yml.
# Use `python -m esptool` with the underscore subcommand/flags spelling
# so we work on both the older esptool that IDF 5.5 venvs ship
# (`merge_bin` / `--flash_mode`) and newer releases that accept either.
python -m esptool --chip esp32s3 merge_bin \
    -o "${stage_dir}/${variant}.bin" \
    --flash_mode dio \
    --flash_freq 80m \
    --flash_size "$flash_size" \
    0x0      "${stage_dir}/bootloader.bin" \
    0x8000   "${stage_dir}/partitions.bin" \
    0xe000   "${stage_dir}/ota_data_initial.bin" \
    0x10000  "${stage_dir}/firmware.bin" \
    "$fs_offset_hex" "$fs_bin"

# ---- 6. Per-artifact sha256 ----
( cd "$stage_dir" && for f in *.bin; do
    sha256sum "$f" | awk '{print $1}' > "${f}.sha256"
done )

echo
echo "Staged ${variant} artifacts in ${stage_dir}:"
ls -la "$stage_dir"
