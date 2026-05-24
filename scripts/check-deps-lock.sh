#!/usr/bin/env bash
# Fails if dependencies.lock contains absolute `path:` entries for local
# components. The IDF Component Manager records absolute paths when local
# overrides (override_path) are resolved, which is not portable across
# machines. Espressif's own docs recommend not committing the lock in
# that case, but we want to keep registry version pinning, so the rule
# we enforce is: lock may be committed, but local paths in it must be
# relative.
set -euo pipefail

LOCK="${1:-dependencies.lock}"

if [ ! -f "$LOCK" ]; then
  echo "check-deps-lock: $LOCK not found" >&2
  exit 2
fi

# Match `  path: /...` but ignore `registry_url: https://...` etc.
bad="$(grep -nE '^[[:space:]]*path:[[:space:]]*/' "$LOCK" || true)"

if [ -n "$bad" ]; then
  echo "check-deps-lock: absolute path(s) found in $LOCK:" >&2
  echo "$bad" >&2
  echo >&2
  echo "Rewrite each path: entry to be relative to the project root" >&2
  echo "(e.g. main/component_overrides/<name>) and commit again." >&2
  exit 1
fi
