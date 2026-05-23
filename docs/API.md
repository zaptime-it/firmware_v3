# HTTP API

Machine-readable, authoritative: [`data/static/swagger.yml`](../data/static/swagger.yml)
and [`data/static/swagger.json`](../data/static/swagger.json). This page
is a hand-written summary that matches the shape of the firmware as of
3.4.0. If you add, remove, or change a route, update both the firmware
and the swagger file in the same change.

## Conventions

- **Verbs.** `GET` for reads, `POST` for state changes, `PATCH` for
  partial settings updates. The legacy 3.3.x-era `GET` verbs for state
  changes (`GET /api/identify`, `GET /api/full_refresh`, etc.) and the
  `POST /api/json/settings` endpoint are no longer registered.
- **Auth.** When the NVS key `httpAuthEnabled` is `true`, every
  endpoint listed here requires HTTP Basic auth, including the SSE
  stream and read-only status endpoints. When it is `false` — the
  factory default — no endpoint requires auth.
- **CORS.** `Access-Control-Allow-Origin: *`, `Allow-Methods: GET,
  PATCH, POST, OPTIONS`, `Allow-Headers: Content-Type, Authorization`.
  `*` for headers would combine badly with the permissive origin, so
  the allowed header set is explicit.
- **Content type.** Request bodies for `PATCH /api/settings`,
  `POST /api/show/custom`, and `POST /api/lights/set` are JSON. All
  other POST bodies are empty or use query parameters.
- **Path rewrites.** A handful of endpoints register a path-parameter
  rewrite so `/foo/{x}` maps to `/foo?x=`. This is purely ergonomic; the
  query-parameter form is the "real" route.
- **Status codes.** `200` success, `400` bad request, `401`
  unauthenticated, `404` unknown endpoint / unknown parameter, `503`
  already in progress (OTA).

## Status & SSE

| Method | Path                  | Purpose                                                      |
| ------ | --------------------- | ------------------------------------------------------------ |
| GET    | `/events`             | Server-Sent-Events stream. Emits `welcome` on connect and `status` on every device state change. Payload shape == `GET /api/status`. |
| GET    | `/api/status`         | Current display content, screen index, timer state, data-source connection matrix, DND state, LED colours, frontlight array. |
| GET    | `/api/system_status`  | Heap / PSRAM / LittleFS / WiFi diagnostics only.             |

The SSE `status` event and `GET /api/status` share a single builder
(`buildStatusJson()` in `net/webserver/status.cpp`), so they cannot
drift.

## Settings

| Method | Path            | Purpose                                                       |
| ------ | --------------- | ------------------------------------------------------------- |
| GET    | `/api/settings` | Full settings snapshot. Passwords are replaced by boolean flags (see below). |
| PATCH  | `/api/settings` | Partial update. Body is a JSON object; only present fields are written. |

`GET /api/settings` never returns plaintext passwords. Instead it
returns:

- `httpAuthPassSet: bool` — `true` if a non-empty `httpAuthPass` is
  stored (not just because a default exists).
- `otaPassSet: bool` — `true` if a non-empty `otaPass` is stored.

Both can be cleared by `PATCH`-ing an empty string, and set by
`PATCH`-ing the desired value. The UI should show a masked "•••" input
when the flag is true, and a normal input with the current effective
default when it is false.

Settings are a flat map of `NVS key → value`. The canonical key
inventory is [`main/lib/system/pref_keys.hpp`](../main/lib/system/pref_keys.hpp);
see [PREFERENCES.md](PREFERENCES.md). The `dnd` sub-object is the only
nested element, and it maps onto `LedHandler`'s DND time window:

```json
{
  "timerSeconds": 30,
  "dataSource": 2,
  "ceEndpoint": "https://v2.btclock.dev",
  "dnd": {
    "dndTimeEnabled": true,
    "startHour": 22,
    "startMinute": 0,
    "endHour": 7,
    "endMinute": 30
  }
}
```

The legacy key `customEndpoint` has been retired in 3.4.0; use
`ceEndpoint` instead. There is no migration shim — fresh installs and
upgraded installs both treat `customEndpoint` as unknown.

## Actions (state changes)

All `POST`. All gated by `requireHttpAuth()`. Successful calls return
`200 OK` with an empty body unless noted.

| Path                          | Body / params                     | Effect                                                      |
| ----------------------------- | --------------------------------- | ----------------------------------------------------------- |
| `/api/identify`               | —                                 | Flashes the LED ring in a distinctive pattern.              |
| `/api/restart`                | —                                 | Reboots after the response is flushed.                      |
| `/api/full_refresh`           | —                                 | Forces a full e-paper refresh.                              |
| `/api/action/pause`           | —                                 | Pause the screen rotation timer.                            |
| `/api/action/timer_restart`   | —                                 | Resume the screen rotation timer.                           |
| `/api/stop_datasources`       | —                                 | Stop BlockNotify + PriceNotify.                             |
| `/api/restart_datasources`    | —                                 | Restart the active data source(s).                           |
| `/api/screen/next`            | —                                 | Advance to the next screen.                                 |
| `/api/screen/previous`        | —                                 | Step back.                                                  |
| `/api/show/screen`            | `?s={id}` (or `/api/show/screen/{id}`)   | Jump to a specific screen id.                        |
| `/api/show/currency`          | `?c={code}` (or `/api/show/currency/{code}`) | Switch the displayed currency.                   |
| `/api/show/text`              | `?t={text}` (or `/api/show/text/{text}`)     | Render `{text}` across the display.              |
| `/api/show/number/{number}`   | path param                        | Same as `show/text` with a number literal.                   |
| `/api/show/custom`            | JSON array of strings             | One string per screen (≤ NUM_SCREENS).                       |
| `/api/wifi_set_tx_power`      | `?txPower={n}`                    | Set Wi-Fi TX power. Persisted to NVS on success.             |

## Lights & frontlight

| Method | Path                         | Params / body         | Notes                                    |
| ------ | ---------------------------- | --------------------- | ---------------------------------------- |
| GET    | `/api/lights`                | —                     | Current per-LED colour array.            |
| POST   | `/api/lights/off`            | —                     | Turn all LEDs off.                       |
| POST   | `/api/lights/color`          | `?c={HEX}` (or `/api/lights/color/{color}`) | Set all LEDs to one colour. |
| POST   | `/api/lights/set`            | JSON array            | Per-LED RGB or hex colour.               |
| GET    | `/api/frontlight/status`     | —                     | Per-panel frontlight PWM array.          |
| POST   | `/api/frontlight/on`         | —                     | Fade in all panels.                      |
| POST   | `/api/frontlight/off`        | —                     | Fade out all panels.                     |
| POST   | `/api/frontlight/flash`      | —                     | Flash the configured effect.             |
| POST   | `/api/frontlight/brightness` | `?b={0-65535}`        | Set overall brightness.                  |

`/api/frontlight/*` routes are only registered when the build defines
`HAS_FRONTLIGHT` (Rev B only).

## Do Not Disturb

DND suppresses LED effects without changing the displayed data. Two
independent flags exist: a manual override (`enabled`) and a
time-based schedule (`dndTimeEnabled` + `startTime`/`endTime` on the
status payload).

| Method | Path                | Notes                             |
| ------ | ------------------- | --------------------------------- |
| GET    | `/api/dnd/status`   | Current DND state + time window.  |
| POST   | `/api/dnd/enable`   | Manual override on.               |
| POST   | `/api/dnd/disable`  | Manual override off.              |

Time-based settings (start/end hour/minute, `dndTimeEnabled`) are
updated via `PATCH /api/settings` with a `dnd: {…}` object. The pure
time-range algebra lives in `lib/btclock/dnd_window.hpp` and has its own
unit test suite ([TESTING.md#dnd-time-window](TESTING.md#dnd-time-window)).

## OTA

| Method | Path                        | Body                          | Notes                                    |
| ------ | --------------------------- | ----------------------------- | ---------------------------------------- |
| POST   | `/upload/firmware`          | multipart firmware image      | Streams directly into `Update.write()`.  |
| POST   | `/upload/webui`             | multipart LittleFS image      | Streams directly into `Update.write()`.  |
| POST   | `/api/firmware/auto_update` | —                             | Kick the `otaTask` to pull `gitReleaseUrl`. Returns `503` if an update is already in progress. |

The automatic update path and the manual upload path both use the same
streaming SHA-256 pipeline, so neither ever holds the full firmware
image in heap.

ArduinoOTA push is also enabled when `otaEnabled` is true. The password
(`otaPass` NVS key), when non-empty, is applied before
`ArduinoOTA.begin()`. Leave blank to disable password auth.

## Errors

| Status | When                                                                 |
| -----: | -------------------------------------------------------------------- |
| `200`  | Success.                                                             |
| `400`  | Bad query param (`wifi_set_tx_power` out of range, malformed hex in `lights/set`, `lights/set` array length mismatch, missing required param on `frontlight/brightness`). |
| `401`  | HTTP Basic auth enabled and the caller didn't pass valid credentials. |
| `404`  | Unknown path (including `/api` paths without `OPTIONS`), unknown screen id, currency not in `actCurrencies`. |
| `503`  | OTA already in progress for `/api/firmware/auto_update`.             |

## Compatibility

3.4.0 is a breaking change relative to 3.3.x. The WebUI bundled with
the firmware is developed in lockstep: the firmware's `data/` submodule
`feature/3.4.0` branch is the only client that is expected to work
against the new endpoint set. External integrations that were hitting
the old `GET` state-change endpoints need to be updated to `POST`, and
any integration that was using `POST /api/json/settings` needs to move
to `PATCH /api/settings`.
