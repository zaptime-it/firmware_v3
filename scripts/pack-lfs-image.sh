#!/usr/bin/env bash
# Pack one LittleFS image for the WebUI bundle at one of the three
# supported flash sizes (4MB / 8MB / 16MB). The CI release pipeline runs
# this once per size and shares the resulting image across every
# firmware variant that targets that flash budget (e.g. the two Rev A
# variants both consume littlefs_4MB.bin).
#
# Run from anywhere; cd's to repo root automatically.
#
# Usage:
#   ./scripts/pack-lfs-image.sh <4MB|8MB|16MB> <out-dir>
#
# Inputs:
#   - data/build_gz/www/ must already exist (pnpm build + gzip_build.py
#     have run); we don't rebuild the WebUI here.
#   - littlefs-python must be on $PATH (pip install littlefs-python==0.15.0).

set -euo pipefail

flash_size="${1:?flash size required (4MB | 8MB | 16MB)}"
out_dir="${2:?output directory required}"

cd "$(dirname "$0")/.."
repo_root="$(pwd)"
data_root="${repo_root}/data/build_gz/www"

case "$flash_size" in
    4MB)  partition_table="${repo_root}/partition.csv" ;;
    8MB)  partition_table="${repo_root}/partition_8mb.csv" ;;
    16MB) partition_table="${repo_root}/partition_16mb.csv" ;;
    *)
        echo "Unknown flash size '$flash_size' (expected 4MB | 8MB | 16MB)" >&2
        exit 64
        ;;
esac

if [[ ! -d "$data_root" ]]; then
    echo "WebUI build output missing: $data_root" >&2
    echo "Run 'cd data && pnpm install && pnpm build && python3 gzip_build.py' first." >&2
    exit 65
fi

fs_size_hex=$(awk -F',' '
    /^[[:space:]]*spiffs[[:space:]]*,/ {
        gsub(/^[[:space:]]+|[[:space:]]+$/, "", $5)
        print $5
        exit
    }' "$partition_table")
if [[ -z "$fs_size_hex" ]]; then
    echo "Could not find spiffs partition size in $partition_table" >&2
    exit 66
fi

# Trim to whole blocks — esp_littlefs at runtime drops any partial
# trailing block (block_count = partition_size / 4096, integer div).
# The image's on-disk block_count must equal what runtime expects or
# lfs_mount fails with LFS_ERR_INVAL.
fs_blocks=$(( fs_size_hex / 4096 ))
fs_image_size=$(( fs_blocks * 4096 ))

mkdir -p "$out_dir"
out_bin="${out_dir}/littlefs_${flash_size}.bin"

# CONFIG_LITTLEFS_OBJ_NAME_LEN=64 in sdkconfig.defaults — the image's
# on-disk name_max must match or the runtime mount fails.
littlefs-python create "$data_root" "$out_bin" -v \
    --fs-size="$fs_image_size" \
    --name-max=64 \
    --block-size=4096

echo "Packed ${out_bin} (${fs_image_size} bytes, ${fs_blocks} blocks)"
