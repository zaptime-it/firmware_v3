cmake_minimum_required(VERSION 3.16.0)

# v3 LTS experimental IDF-as-build flow with Arduino as a managed
# component. Lives in idf-build/ so platformio.ini-based Rev B/V8
# builds remain untouched. Run via:
#   source ~/.platformio/packages/framework-espidf/export.sh
#   cd idf-build
#   python3 fetch_arduino_libs.py     # one-off, idempotent
#   idf.py -DSDKCONFIG_DEFAULTS='sdkconfig.defaults;sdkconfig.defaults.lolin_s3_mini_213epd' \
#          -B build_lolin_mini_213 build

# Arduino libraries that aren't published to the IDF Component Registry
# are vendored into arduino_libraries/<name>/ by fetch_arduino_libs.py.
# Each is exposed to the build as a normal IDF component.
list(APPEND EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}/arduino_libraries")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(btclock_v3_lts)
