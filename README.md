# BTClock v3

[![Latest release](https://git.btclock.dev/btclock/btclock_v3/badges/release.svg)](https://git.btclock.dev/btclock/btclock_v3/releases/latest)

[![BTClock CI](https://git.btclock.dev/btclock/btclock_v3/badges/workflows/push.yaml/badge.svg)](https://git.btclock.dev/btclock/btclock_v3/actions?workflow=push.yaml&actor=0&status=0)

Software for the BTClock project.

Biggest differences with v2 are:
- Uses WebSockets for all data
- Built on the ESP-IDF with Arduino as a library 
- Makes better use of native timers and interrupts
- Able to be flashed over-the-air (using ESP OTA)
- Added market capitalization screen
- LED flash on new block (and focus to block height screen on new block)

New features:
- BitAxe integration
- Nostr Zap notifier
- Multiple mining pool stats integrations

"Steal focus on new block" means that when a new block is mined, the display will switch to the block height screen if it's not on it already.

See the [docs](https://git.btclock.dev/btclock/docs) repo for more information and building instructions.

**NOTE**: The software assumes that the hardware is run in a controlled private network. ~~The Web UI and the OTA update mechanism are not password protected and accessible to anyone in the network. Also, since the device only fetches numbers through WebSockets it will skip server certificate verification to save resources.~~ Since 3.2.0 the WebUI is password protectable and all certificates are verified. OTA update mechanism is not password-protected. 

## Building

Use PlatformIO to build it yourself. Make sure you fetch the [WebUI](https://git.btclock.dev/btclock/webui) submodule.


## Mining pool stats
Enable mining pool stats by accessing your btclock's web UI (point a web browser at the device's IP address).

Under Settings -> Extra Features: toggle Enable Mining Pool Stats.

New options will appear. Select your mining pool and enter your pool username (Ocean) or api key (Braiins).

The Mining Pool Earnings screen displays:
* Braiins: Today's mining reward thus far
* Ocean: Your estimated earnings if the pool were to find a block right now

For solo mining pools, there are no earning estimations. Your username is the onchain withdrawal address, without the worker name.


### Braiins Pool integration
Create an API key based on the steps [here](https://academy.braiins.com/en/braiins-pool/monitoring/#api-configuration).

The key's permissions should be:
* Web Access: no
* API Access: yes
* Access Permissions: Read-only

Copy the token that is created for the new key. Enter this as your "Mining Pool username or api key" in the btclock web UI.


### Ocean integration
Your "Mining Pool username" is just the onchain withdrawal address that you specify when pointing your miners at Ocean.
