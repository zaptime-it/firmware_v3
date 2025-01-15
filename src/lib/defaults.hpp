#pragma once

#define INITIAL_BLOCK_HEIGHT 876600
#define INITIAL_LAST_PRICE 50000
#define DEFAULT_TX_POWER 0

#define DEFAULT_MEMPOOL_SECURE true
#define DEFAULT_LED_TEST_ON_POWER true
#define DEFAULT_LED_FLASH_ON_UPD false
#define DEFAULT_LED_BRIGHTNESS 128
#define DEFAULT_STEAL_FOCUS false
#define DEFAULT_MCAP_BIG_CHAR true
#define DEFAULT_MDNS_ENABLED true
#define DEFAULT_OTA_ENABLED true
#define DEFAULT_FETCH_EUR_PRICE false
#define DEFAULT_USE_SATS_SYMBOL false
#define DEFAULT_USE_BLOCK_COUNTDOWN true
#define DEFAULT_SUFFIX_PRICE false
#define DEFAULT_DISABLE_LEDS false
#define DEFAULT_DISABLE_FL false
#define DEFAULT_MOW_MODE false
#define DEFAULT_SUFFIX_SHARE_DOT false

#define DEFAULT_V2_SOURCE_CURRENCY CURRENCY_USD

#define DEFAULT_TIME_OFFSET_SECONDS 3600

#define DEFAULT_HOSTNAME_PREFIX "btclock"
#define DEFAULT_MEMPOOL_INSTANCE "mempool.space"

#define DEFAULT_USE_NOSTR false
#define DEFAULT_NOSTR_NPUB "642317135fd4c4205323b9dea8af3270657e62d51dc31a657c0ec8aab31c6288"
#define DEFAULT_NOSTR_RELAY "wss://relay.primal.net"

#define DEFAULT_SECONDS_BETWEEN_PRICE_UPDATE 30
#define DEFAULT_MINUTES_FULL_REFRESH 60

#define DEFAULT_FG_COLOR GxEPD_WHITE
#define DEFAULT_BG_COLOR GxEPD_BLACK

#define DEFAULT_WP_TIMEOUT 15*60

#define DEFAULT_FL_MAX_BRIGHTNESS 2048
#define DEFAULT_FL_EFFECT_DELAY 15

#define DEFAULT_LUX_LIGHT_TOGGLE 128
#define DEFAULT_FL_OFF_WHEN_DARK true   

#define DEFAULT_FL_ALWAYS_ON true
#define DEFAULT_FL_FLASH_ON_UPDATE true

#define DEFAULT_LED_STATUS false
#define DEFAULT_TIMER_ACTIVE true
#define DEFAULT_TIMER_SECONDS 1800
#define DEFAULT_CURRENT_SCREEN 0

#define DEFAULT_BITAXE_ENABLED false
#define DEFAULT_BITAXE_HOSTNAME "bitaxe1"

#define DEFAULT_MINING_POOL_STATS_ENABLED false
#define DEFAULT_MINING_POOL_NAME "ocean"
#define DEFAULT_MINING_POOL_USER "38Qkkei3SuF1Eo45BaYmRHUneRD54yyTFy"  // Random actual Ocean hasher
#define DEFAULT_LOCAL_POOL_ENDPOINT "umbrel.local:2019"

#define DEFAULT_ZAP_NOTIFY_ENABLED false
#define DEFAULT_ZAP_NOTIFY_PUBKEY "b5127a08cf33616274800a4387881a9f98e04b9c37116e92de5250498635c422"
#define DEFAULT_LED_FLASH_ON_ZAP true
#define DEFAULT_FL_FLASH_ON_ZAP true
#define DEFAULT_FONT_NAME "antonio"

#define DEFAULT_HTTP_AUTH_ENABLED false
#define DEFAULT_HTTP_AUTH_USERNAME "btclock"
#define DEFAULT_HTTP_AUTH_PASSWORD "satoshi"

#define DEFAULT_ACTIVE_CURRENCIES "USD,EUR,JPY"

#define DEFAULT_GIT_RELEASE_URL "https://git.btclock.dev/api/v1/repos/btclock/btclock_v3/releases/latest"
#define DEFAULT_VERTICAL_DESC true

#define DEFAULT_MINING_POOL_LOGOS_URL "https://git.btclock.dev/btclock/mining-pool-logos/raw/branch/main"

#define DEFAULT_ENABLE_DEBUG_LOG false

#define DEFAULT_DISABLE_FL false
#define DEFAULT_CUSTOM_ENDPOINT "ws-staging.btclock.dev"
#define DEFAULT_CUSTOM_ENDPOINT_DISABLE_SSL false
#define DEFAULT_MOW_MODE false

// Define data source types
enum DataSourceType {
    BTCLOCK_SOURCE = 0,  // BTClock's own data source
    THIRD_PARTY_SOURCE = 1,  // Third party data sources like mempool.space
    NOSTR_SOURCE = 2,  // Nostr data source
    CUSTOM_SOURCE = 3  // Custom data source endpoint
};

#define DEFAULT_DATA_SOURCE BTCLOCK_SOURCE

#ifndef DEFAULT_BOOT_TEXT
#define DEFAULT_BOOT_TEXT "BTCLOCK"
#endif