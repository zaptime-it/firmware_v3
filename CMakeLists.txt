cmake_minimum_required(VERSION 3.16.0)

# BTClock v3 firmware project (ESP-IDF 5.5 + Arduino-ESP32 3.3 as a
# managed component). One-shot build via the wrapper scripts:
#
#   ./scripts/build.sh                  # build all 4 variants
#   ./scripts/build.sh <variant>        # build one
#   ./scripts/build-release.sh <variant># build + LittleFS + esptool merge_bin
#
# Or the underlying idf.py call directly (from repo root):
#
#   source $IDF_PATH/export.sh
#   python3 scripts/fetch_arduino_libs.py     # one-off, idempotent
#   idf.py -DSDKCONFIG_DEFAULTS='sdkconfig.defaults;sdkconfig.defaults.<variant>' \
#          -DBTCLOCK_VARIANT=<variant> -DIDF_TARGET=esp32s3 \
#          -B build_<variant> build

# Arduino libraries that aren't published to the IDF Component Registry
# are vendored into arduino_libraries/<name>/ by scripts/fetch_arduino_libs.py.
# Each is exposed to the build as a normal IDF component.
list(APPEND EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}/arduino_libraries")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(btclock_v3)

# Make Arduino's USB defines visible to every component, not just main.
# HWCDC.cpp inside the espressif__arduino-esp32 component defines
# `HWCDCSerial` behind #if ARDUINO_USB_MODE && ARDUINO_USB_CDC_ON_BOOT;
# our main lib uses `Serial.println(...)` which the HardwareSerial.h
# resolver maps to `HWCDCSerial` under the same #if. If the defines only
# reach main, HWCDC.cpp compiles the symbol out and the link fails with
# `undefined reference to 'HWCDCSerial'`. Setting them on the global
# build property propagates them to every component build line.
idf_build_set_property(COMPILE_DEFINITIONS "-DARDUINO_USB_MODE=1" APPEND)
idf_build_set_property(COMPILE_DEFINITIONS "-DARDUINO_USB_CDC_ON_BOOT=1" APPEND)
