# Contributing

Quick orientation for contributors. None of this is mandatory, but
matching the conventions keeps reviews short.

## Setup

See [BUILD.md](BUILD.md) for the full toolchain install. TL;DR:

```bash
git clone --recurse-submodules https://git.btclock.dev/btclock/btclock_v3.git
cd btclock_v3
# ESP-IDF v5.5 (one-time):
git clone --depth 1 --branch v5.5 --recurse-submodules --shallow-submodules \
  https://github.com/espressif/esp-idf.git ~/esp/esp-idf
~/esp/esp-idf/install.sh esp32s3
source ~/esp/esp-idf/export.sh
# Sanity:
cmake -G Ninja -B build-tests -S tests && cmake --build build-tests -j && \
  ctest --test-dir build-tests --output-on-failure
./scripts/build.sh                  # build all four shipping variants
```

## Before you push

```bash
ctest --test-dir build-tests --output-on-failure   # host tests
./scripts/build.sh lolin_s3_mini_213epd           # tightest flash budget
./scripts/build.sh                                # the other three variants
```

If you changed a handler, a header, or a setting, also manually
smoke-test against a real device on at least one of:

- `lolin_s3_mini_213epd` (4 MB flash, 2 MB PSRAM — the tight one)
- `btclock_rev_b_213epd`  (8 MB flash, 8 MB PSRAM — the shipping one)

## Code conventions

- **One directory per concern under `main/lib/`.** See
  [ARCHITECTURE.md#tree-layout](ARCHITECTURE.md#tree-layout). Don't
  drop new files into the flat root.
- **NVS keys are `PrefKeys::…` constants.** No inline `"stringLiteral"`
  keys. See [PREFERENCES.md](PREFERENCES.md).
- **Defaults are `DEFAULT_*` constants** in
  `main/lib/system/defaults.hpp`.
- **Verbs are `HTTP_GET` / `HTTP_POST` / `HTTP_PATCH`.** Don't add
  state-changing `HTTP_GET` routes. See [API.md](API.md).
- **Every state-changing handler calls `requireHttpAuth(request)` at
  the top and bails early on `true`.** Read-only endpoints that reveal
  live status (`/api/status`, `/events`) call it too.
- **ISRs must not dereference flash-resident singletons.** Cache
  `TaskHandle_t` in a `static volatile` at setup time and null-check
  before `vTaskNotifyGiveFromISR`. See `main/lib/system/timers.cpp` for
  the pattern.
- **Shared state across tasks is `std::atomic` or mutex-protected.**
  `BlockNotify` uses `std::atomic<>` for its scalar statics;
  `PriceNotify` uses a `std::mutex` for its maps. Don't reach for
  FreeRTOS mutexes when the C++ primitives fit.
- **HTTP clients use `HttpHelper::beginScoped()`** from
  `main/lib/system/shared.hpp`. Raw `HTTPClient` is a code-review
  comment.
- **Comments explain *why*, not *what*.** If the diff needs a
  paragraph of rationale, put it in the commit message first and
  quote the short form as a comment.

## Commit messages

- One logical change per commit.
- Subject line ≤ 72 chars, imperative mood ("Add X", not "Added X").
- The body wraps at ~72 chars and explains the motivation. "What"
  should be obvious from the diff; the commit is the place to write
  down "why this was worth changing".
- No `Made-with: Cursor` trailers; no tool attribution in general.

## Plan file

[`README.md`](../README.md) in the repo root points at the
`btclock-firmware-overhaul` plan file for the 3.4.0 refactor. That file
records, commit-by-commit, how the codebase reached its current shape
(Phases 1 – 9). Read it if you want the history; don't feel obliged to
read it to contribute.

## Changing the API

API changes are breaking if they touch any URL, verb, query parameter,
or JSON shape the WebUI reads. When you make one:

1. Change the firmware route.
2. Update `data/static/openapi.yml` *and* `data/static/openapi.json`.
3. Update the matching WebUI types and client in `data/main/lib/api/`.
4. Update the relevant section of [API.md](API.md).
5. If you retired a key, remove its `PrefKeys::` constant and any
   migration shim. See [PREFERENCES.md#legacy-keys](PREFERENCES.md#legacy-keys).

The firmware branch and the WebUI branch share a name during a major
push (e.g. both are `feature/3.4.0`) so the submodule bump is a
one-liner at the end.

## Questions

Open an issue on the Forgejo instance at
<https://git.btclock.dev/btclock/btclock_v3/issues> or ping the
maintainers in the BTClock Nostr channel.
