# Glob the existing PIO src/ tree from one level up. Keeping all source
# in ../src/ means the PIO build can keep using it unchanged; we just
# point IDF's main component at the same files. INCLUDE_DIRS mirrors
# what platformio.ini exposed implicitly through Arduino + ESP-IDF.

set(SRC_ROOT "${CMAKE_CURRENT_LIST_DIR}/../src")
set(INC_ROOT "${CMAKE_CURRENT_LIST_DIR}/../include")

file(GLOB_RECURSE app_sources
    "${SRC_ROOT}/*.cpp"
    "${SRC_ROOT}/*.c"
    # PIO built every lib/<dir>/*.cpp as a static lib via lib_extra_dirs.
    # Pull them into main so symbols (parseBlockFees, getCurrencyCode,
    # btclock::parseScreenOrder, etc.) resolve at link time.
    "${CMAKE_CURRENT_LIST_DIR}/../lib/btclock/*.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../lib/btclock/*.c"
    "${CMAKE_CURRENT_LIST_DIR}/../lib/qrcode/*.c"
)

idf_component_register(
    SRCS ${app_sources}
    INCLUDE_DIRS
        "${SRC_ROOT}"
        "${INC_ROOT}"
        "${CMAKE_CURRENT_LIST_DIR}/../lib"
        # PIO auto-added every subdir of lib/ as an include path
        # (each is treated as an Arduino lib root). IDF doesn't, so
        # list them explicitly. lib/btclock holds utils.hpp; lib/qrcode
        # is the in-tree qrcodegen vendor.
        "${CMAKE_CURRENT_LIST_DIR}/../lib/btclock"
        "${CMAKE_CURRENT_LIST_DIR}/../lib/qrcode"
    # No EMBED_FILES for x509_crt_bundle: the IDF flow uses the
    # default Mozilla bundle that esp-tls + mbedtls auto-generate
    # (CONFIG_MBEDTLS_CERTIFICATE_BUNDLE_DEFAULT_CMN=y in
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

# ---- Build identity (replaces scripts/git_rev.py + -DLAST_BUILD_TIME=$UNIX_TIME) ----
# Both regenerate every configure, matching PIO. If a reproducible build
# is wanted, override on the idf.py command line with
# -DBTCLOCK_GIT_REV="..." -DBTCLOCK_BUILD_TIME=0.
if(NOT DEFINED BTCLOCK_GIT_REV)
    execute_process(
        COMMAND git rev-parse --short HEAD
        WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/.."
        OUTPUT_VARIABLE BTCLOCK_GIT_REV
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(NOT BTCLOCK_GIT_REV)
        set(BTCLOCK_GIT_REV "unknown")
    endif()
endif()
if(NOT DEFINED BTCLOCK_BUILD_TIME)
    string(TIMESTAMP BTCLOCK_BUILD_TIME "%s")
endif()

# ---- Cross-variant defines (every BTClock build needs these) ----
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    CORE_DEBUG_LEVEL=0
    CONFIG_ASYNC_TCP_STACK_SIZE=16384
    WEBSOCKETS_MAX_DATA_SIZE=32768
    GIT_REV="${BTCLOCK_GIT_REV}"
    LAST_BUILD_TIME=${BTCLOCK_BUILD_TIME}
)

# ---- Variant-specific defines ----
# Driven by the BTCLOCK_VARIANT CMake variable, set by the caller via
#     idf.py -B ../build_<variant> \
#            -DSDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.<variant>" \
#            -DBTCLOCK_VARIANT=<variant> -DIDF_TARGET=esp32s3 build
# Each block mirrors the corresponding [env:<variant>] in the legacy
# platformio.ini so the firmware-side macros stay identical across
# build systems while migrating.
if(NOT DEFINED BTCLOCK_VARIANT)
    message(FATAL_ERROR
        "BTCLOCK_VARIANT not set. Pass -DBTCLOCK_VARIANT=<one of: "
        "lolin_s3_mini_213epd, lolin_s3_mini_29epd, "
        "btclock_rev_b_213epd, btclock_v8_213epd>")
endif()

if(BTCLOCK_VARIANT STREQUAL "lolin_s3_mini_213epd")
    target_compile_definitions(${COMPONENT_LIB} PRIVATE
        # Rev A hardware (Lolin S3 Mini, 4 MB)
        MCP_INT_PIN=8 NEOPIXEL_PIN=34 NEOPIXEL_COUNT=4 NUM_SCREENS=7
        I2C_SDA_PIN=35 I2C_SCK_PIN=36 IS_HW_REV_A
        # 2.13" EPD variant
        USE_QR VERSION_EPD_2_13 HW_REV="REV_A_EPD_2_13"
        # Headroom for the wider 2.13" QR rendering path.
        CONFIG_ARDUINO_MAIN_TASK_STACK_SIZE=16384
    )
elseif(BTCLOCK_VARIANT STREQUAL "lolin_s3_mini_29epd")
    target_compile_definitions(${COMPONENT_LIB} PRIVATE
        # Same Rev A hardware as the 213epd, only the EPD changes.
        MCP_INT_PIN=8 NEOPIXEL_PIN=34 NEOPIXEL_COUNT=4 NUM_SCREENS=7
        I2C_SDA_PIN=35 I2C_SCK_PIN=36 IS_HW_REV_A
        # 2.9" EPD variant — no 16 KB stack bump in the PIO env, so omit.
        USE_QR VERSION_EPD_2_9 HW_REV="REV_A_EPD_2_9"
    )
elseif(BTCLOCK_VARIANT STREQUAL "btclock_rev_b_213epd")
    target_compile_definitions(${COMPONENT_LIB} PRIVATE
        # Rev B board (custom 8 MB), adds frontlight (PCA9685) + LDR (BH1750).
        MCP_INT_PIN=8 NEOPIXEL_PIN=15 NEOPIXEL_COUNT=4 NUM_SCREENS=7
        I2C_SDA_PIN=35 I2C_SCK_PIN=36
        HAS_FRONTLIGHT PCA_OE_PIN=48 PCA_I2C_ADDR=0x42
        IS_HW_REV_B IS_BTCLOCK_REV_B
        # 2.13" EPD variant
        USE_QR VERSION_EPD_2_13 HW_REV="REV_B_EPD_2_13"
        CONFIG_ARDUINO_MAIN_TASK_STACK_SIZE=16384
    )
elseif(BTCLOCK_VARIANT STREQUAL "btclock_v8_213epd")
    target_compile_definitions(${COMPONENT_LIB} PRIVATE
        # V8 board (16 MB with octal PSRAM), 8 panels, dual-MCP, extra
        # SPI pins. I2C bus moved to GPIO 1/2.
        MCP_INT_PIN=4 NEOPIXEL_PIN=5 NEOPIXEL_COUNT=4 NUM_SCREENS=8
        SPI_SDA_PIN=11 SPI_SCK_PIN=12
        I2C_SDA_PIN=1 I2C_SCK_PIN=2
        MCP_RESET_PIN=21
        MCP1_A0_PIN=6 MCP1_A1_PIN=7 MCP1_A2_PIN=8
        MCP2_A0_PIN=9 MCP2_A1_PIN=10 MCP2_A2_PIN=14
        IS_BTCLOCK_V8
        # 2.13" EPD variant (no 29epd flavour of V8 ships)
        USE_QR VERSION_EPD_2_13 HW_REV="REV_V8_EPD_2_13"
    )
else()
    message(FATAL_ERROR "Unknown BTCLOCK_VARIANT '${BTCLOCK_VARIANT}'")
endif()
