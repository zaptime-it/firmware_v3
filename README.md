# BTClock v3

[![Latest release](https://git.btclock.dev/btclock/btclock_v3/badges/release.svg)](https://git.btclock.dev/btclock/btclock_v3/releases/latest)

[![BTClock CI](https://git.btclock.dev/btclock/btclock_v3/badges/workflows/push.yaml/badge.svg)](https://git.btclock.dev/btclock/btclock_v3/actions?workflow=push.yaml&actor=0&status=0)

Software for the BTClock project. Built on the ESP-IDF with Arduino as a library,
using WebSockets for all live data and making heavy use of native timers and
interrupts.

See the [docs](https://git.btclock.dev/btclock/docs) repo for user-facing
information and flashing instructions. Developer docs (architecture, build envs,
API reference, testing, preferences) live in [`docs/`](docs/README.md) in this
repo.

## Features

- **Screens**, each individually toggleable and shown in rotation with a
  configurable interval:
  - Block height
  - Clock
  - Halving countdown
  - Block fee rate
  - Sats per currency
  - BTC ticker (price)
  - Market cap
  - Bitcoin supply
  - Bitaxe hashrate / best difficulty (when Bitaxe is enabled)
  - Mining pool hashrate / earnings (when mining pool stats are enabled)
- **Steal focus on new block** — display jumps to the block height screen
  when a new block is mined, with an LED flash in a color you choose.
- **Multiple data sources**:
  - The default BTClock source
  - Any public [mempool.space](https://mempool.space) instance, including
    self-hosted
  - A Nostr relay
  - A custom WebSocket endpoint that speaks the BTClock data format — see
    [ws-go-server](https://git.btclock.dev/btclock/ws-go-server) for a
    self-hostable reference implementation
- **Bitaxe integration** — fetch hashrate and best difficulty from a local
  Bitaxe miner and show it on its own screens.
- **Mining pool stats** — hashrate and (where the pool exposes it) earnings
  screens. Supported pools:
  - Braiins
  - Ocean
  - Noderunners
  - Satoshi Radio
  - [public-pool.io](https://public-pool.io)
  - Self-hosted Public Pool
  - GoBrrr Pool
  - CKPool
  - EU CKPool
- **Nostr Zap notifier** — flash the LEDs (and frontlight on supported
  hardware) and optionally switch the screen when a zap lands on a given
  pubkey.
- **Frontlight control** — brightness, effect speed, flash on block/zap,
  always-on, and auto-off when the ambient light sensor reports a dark
  room (on hardware that has one).
- **Time-based Do Not Disturb** — dim/disable the display between a set
  start and end time.
- **Customisation** — inverted colors, multiple fonts, LED brightness and
  flash color, currencies shown on the currency-dependent screens, big
  characters for market cap, block countdown mode, sats symbol, price
  suffix formatting, MoW mode (Samson-Mow-style price in millions, e.g.
  `0.1M` instead of `100k`), and more.
- **Multi-language WebUI** — English, Dutch, German, Spanish.
- **Over-the-air updates** — ESP OTA via the WebUI, and ArduinoOTA for
  push updates from PlatformIO.
- **mDNS** discovery so you can reach the device by hostname.

**Security**: the device is meant to run on a trusted private network. Since
3.2.0 the WebUI can be password-protected and the firmware verifies server
certificates on outbound requests. Since 3.4.0 the ArduinoOTA push-update
mechanism can be password-protected too.

## Building

Use PlatformIO to build it yourself. Make sure you fetch the
[WebUI](https://git.btclock.dev/btclock/webui) submodule. Full build
instructions are in [`docs/BUILD.md`](docs/BUILD.md).

## Mining pool stats

Enable mining pool stats from the WebUI under **Settings → Extra Features →
Enable Mining Pool Stats**, then pick your pool and enter your pool username
or API key.

The Mining Pool Earnings screen displays:

- **Braiins**: today's mining reward so far.
- **Ocean**: your estimated earnings if the pool were to find a block right
  now.

For solo pools there are no earnings estimations; your username is the
on-chain withdrawal address you configured at the pool, without the worker
name.

### Braiins Pool integration

Create an API key using the steps
[here](https://academy.braiins.com/en/braiins-pool/monitoring/#api-configuration).
The key's permissions should be:

- Web Access: no
- API Access: yes
- Access Permissions: Read-only

Copy the token and enter it as your "Mining Pool username or api key" in the
WebUI.

### Ocean integration

Your "Mining Pool username" is the on-chain withdrawal address you specify
when pointing your miners at Ocean.

### Self-hosted Public Pool

Select **Local Public Pool** as the mining pool and enter the endpoint of
your node's pool API (for example `umbrel.local:2019`). The WebUI has a
"Test" button that verifies it can reach the instance.
