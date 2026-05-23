# Glob the existing PIO src/ tree from one level up. Keeping all source
# in ../../src/ means the PIO build can keep using it unchanged; we just
# point IDF's main component at the same files. INCLUDE_DIRS mirrors
# what platformio.ini exposed implicitly through Arduino + ESP-IDF.

set(SRC_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../src")
set(INC_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../include")

file(GLOB_RECURSE app_sources
    "${SRC_ROOT}/*.cpp"
    "${SRC_ROOT}/*.c"
    # PIO built every lib/<dir>/*.cpp as a static lib via lib_extra_dirs.
    # Pull them into main so symbols (parseBlockFees, getCurrencyCode,
    # btclock::parseScreenOrder, etc.) resolve at link time.
    "${CMAKE_CURRENT_LIST_DIR}/../../lib/btclock/*.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../../lib/btclock/*.c"
    "${CMAKE_CURRENT_LIST_DIR}/../../lib/qrcode/*.c"
)

idf_component_register(
    SRCS ${app_sources}
    INCLUDE_DIRS
        "${SRC_ROOT}"
        "${INC_ROOT}"
        "${CMAKE_CURRENT_LIST_DIR}/../../lib"
        # PIO auto-added every subdir of lib/ as an include path
        # (each is treated as an Arduino lib root). IDF doesn't, so
        # list them explicitly. lib/btclock holds utils.hpp; lib/qrcode
        # is the in-tree qrcodegen vendor.
        "${CMAKE_CURRENT_LIST_DIR}/../../lib/btclock"
        "${CMAKE_CURRENT_LIST_DIR}/../../lib/qrcode"
    # No EMBED_FILES for x509_crt_bundle: the IDF flow uses the
    # default Mozilla bundle that esp-tls + mbedtls auto-generate
    # (CONFIG_MBEDTLS_CERTIFICATE_BUNDLE_DEFAULT_FULL=y in
    # sdkconfig.defaults), not the project-root custom bundle.
    REQUIRES
        # IDF built-ins we use directly (arduino-esp32 brings the rest
        # transitively via Component Manager).
        app_update
        mbedtls
        spi_flash
        nvs_flash
        esp_wifi
        esp_netif
        esp_event
        esp_https_ota
        esp_http_server
        esp_http_client
        esp-tls
        esp_partition
        esp_lcd
        # ArduinoJson via IDF Component Manager (header-only).
        bblanchon__arduinojson
        # Vendored Arduino libraries (see arduino_libraries.json).
        # Direct deps of BTClock src/.
        MCP23017
        PCA9685
        BH1750
        Adafruit_NeoPixel
        Adafruit_GFX_Library
        Adafruit_BusIO
        WiFiManager
        WebSockets
        AsyncTCP
        ESPAsyncWebServer
        universal_pin
        GxEPD2
        Nostrduino
        uBitcoin
)

target_compile_options(${COMPONENT_LIB} PRIVATE
    -std=gnu++17
    -Wno-error=all
)

# Build-time defines mirrored from platformio.ini for the
# lolin_s3_mini_213epd variant. TODO: make variant-aware via
# sdkconfig.defaults.<variant> Kconfig flags or per-variant
# add_compile_definitions wrappers when wiring up Rev B / V8.
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    # Base
    CORE_DEBUG_LEVEL=0
    CONFIG_ASYNC_TCP_STACK_SIZE=16384
    WEBSOCKETS_MAX_DATA_SIZE=32768
    # PIO supplied these via scripts/git_rev.py and -DLAST_BUILD_TIME=$UNIX_TIME.
    # TODO: regenerate at configure time so binaries are reproducible.
    GIT_REV="idf-spike"
    LAST_BUILD_TIME=0
    # Rev A hardware
    MCP_INT_PIN=8
    NEOPIXEL_PIN=34
    NEOPIXEL_COUNT=4
    NUM_SCREENS=7
    I2C_SDA_PIN=35
    I2C_SCK_PIN=36
    IS_HW_REV_A
    # EPD variant
    USE_QR
    VERSION_EPD_2_13
    HW_REV="REV_A_EPD_2_13"
    CONFIG_ARDUINO_MAIN_TASK_STACK_SIZE=16384
)
