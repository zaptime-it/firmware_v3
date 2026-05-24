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
#   - littlefs-python must be on $PATH (pip install littlefs-python==0.17.1).

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

# SvelteKit bakes PUBLIC_BASE_URL into build/env.js at pnpm-build time
# from $PUBLIC_BASE_URL or data/.env. The device build needs it to be
# the empty string so the WebUI does relative API calls against the
# host it was loaded from. A non-empty value (typically a dev's local
# IP for offline UI development) sneaks through unless we check, and
# the flashed image then sends every /api/* call to a stale IP.
env_js="${data_root}/build/env.js.gz"
if [[ -f "$env_js" ]]; then
    baked="$(gzip -dc "$env_js" | grep -oE '"PUBLIC_BASE_URL":"[^"]*"' || true)"
    if [[ -n "$baked" && "$baked" != '"PUBLIC_BASE_URL":""' ]]; then
        echo "WebUI was built with a non-empty PUBLIC_BASE_URL: $baked" >&2
        echo "Rebuild the bundle for device flashing:" >&2
        echo "  cd data && PUBLIC_BASE_URL= pnpm build && PUBLIC_BASE_URL= python3 gzip_build.py" >&2
        echo "(Empty PUBLIC_BASE_URL keeps API calls relative to the device host.)" >&2
        exit 68
    fi
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

# /fs_hash.txt on the LittleFS root identifies the WebUI bundle revision
# — getFsRev() in main/lib/system/config.cpp reads it and exposes the
# value as `fsRev` on /api/settings, which the WebUI's update-checker
# diffs against the latest release. Without this file fsRev comes back
# empty and the "WebUI update available" badge never fires.
#
# data/'s own gzip_build.py writes the gzipped assets to
# data/build_gz/www/ but does NOT write fs_hash.txt; that step lives in
# the data submodule's CI workflow (data/.forgejo/workflows/build.yaml).
# When we pack from build_gz/www/ here, the hash file gets skipped.
# Re-derive it from the data/ submodule's current HEAD instead so a
# local pack matches what the data CI would have written.
hash_file="${data_root}/fs_hash.txt"
data_sha=""
if git -C "${repo_root}/data" rev-parse --verify HEAD >/dev/null 2>&1; then
    data_sha="$(git -C "${repo_root}/data" rev-parse HEAD)"
fi
if [[ -n "$data_sha" ]]; then
    printf "%s" "$data_sha" > "$hash_file"
    echo "fs_hash.txt = $data_sha"
else
    echo "WARNING: data/ is not a git repo; fs_hash.txt will be empty" >&2
    printf "" > "$hash_file"
fi

# CONFIG_LITTLEFS_OBJ_NAME_LEN=64 in sdkconfig.defaults — the image's
# on-disk name_max must match or the runtime mount fails.
littlefs-python create "$data_root" "$out_bin" -v \
    --fs-size="$fs_image_size" \
    --name-max=64 \
    --block-size=4096

echo "Packed ${out_bin} (${fs_image_size} bytes, ${fs_blocks} blocks)"
