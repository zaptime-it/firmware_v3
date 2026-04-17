# BTClock firmware documentation

Developer-facing reference for the BTClock v3 firmware. End-user / flashing
instructions live in the top-level [README.md](../README.md) and at
<https://git.btclock.dev/btclock/docs>.

This folder is organised around questions a new contributor is likely to
ask:

| If you want to know…                               | Read                              |
| -------------------------------------------------- | --------------------------------- |
| How the code is laid out and why                   | [ARCHITECTURE.md](ARCHITECTURE.md) |
| How data flows from the network to the displays   | [ARCHITECTURE.md](ARCHITECTURE.md) |
| How to build for a given hardware variant          | [BUILD.md](BUILD.md)               |
| Which flash / RAM / PSRAM / OTA budget you have    | [BUILD.md](BUILD.md)               |
| What HTTP endpoints the device exposes             | [API.md](API.md)                   |
| Where the authoritative OpenAPI spec lives         | [API.md](API.md)                   |
| How to run or add unit tests                       | [TESTING.md](TESTING.md)           |
| How CI is wired on Forgejo and GitHub              | [TESTING.md](TESTING.md)           |
| The NVS (`Preferences`) key inventory policy       | [PREFERENCES.md](PREFERENCES.md)   |

None of these documents are generated — they are hand-written and pinned
to the state of `main`. The [plan file](../README.md) for the 3.4.0
refactor (Phases 1 – 9) is the commit-by-commit record of how the code
reached this layout; this folder is the "after" picture only.
