# Host-only unit tests for BTClock v3. The test sources alongside this
# file (test_<name>/test_main.cpp) use Unity assertions and run on the
# host with no Arduino / FreeRTOS / ESP-IDF runtime.
#
# Usage (run from repo root):
#   cmake -B .builds/tests       -S tests
#   cmake -B .builds/tests-asan  -S tests -DBTCLOCK_TEST_SANITIZE=ON
#   cmake --build .builds/tests -j
#   ctest --test-dir .builds/tests --output-on-failure
#
# Each tests/test_<name>/test_main.cpp becomes one ctest target. Every
# test links the full lib/btclock/ pile because the headers are small
# and entangled (utils.hpp is everywhere) and the unused symbols get
# stripped at link time anyway.

cmake_minimum_required(VERSION 3.16)
project(btclock_v3_host_tests CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)  # gnu++17, matches the firmware build
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

enable_testing()

# ---- Unity (ThrowTheSwitch) ----
# Pinned to a known-good release. Bumping is safe as long as the
# TEST_ASSERT_* macros stay source-compatible.
include(FetchContent)
FetchContent_Declare(
    unity
    GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
    GIT_TAG v2.6.0
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(unity)

# ---- Optional ASan + UBSan (BTCLOCK_TEST_SANITIZE=ON) ----
option(BTCLOCK_TEST_SANITIZE "Build host tests with ASan + UBSan" OFF)
if(BTCLOCK_TEST_SANITIZE)
    add_compile_options(-O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined)
    add_link_options(-fsanitize=address,undefined)
endif()

# ---- Project sources reachable from host tests ----
# Everything under lib/btclock/ was extracted specifically to be
# host-buildable (no Arduino, no IDF, no FreeRTOS — pure C++/STL).
set(BTCLOCK_REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/..")
file(GLOB BTCLOCK_LIB_SOURCES "${BTCLOCK_REPO_ROOT}/lib/btclock/*.cpp")

# Test discovery: each tests/test_<name>/test_main.cpp becomes one
# executable + ctest entry. Add new tests by dropping a new
# test_<name>/test_main.cpp into tests/; no edits here required.
file(GLOB TEST_DIRS LIST_DIRECTORIES TRUE "${CMAKE_CURRENT_LIST_DIR}/test_*")

foreach(test_dir ${TEST_DIRS})
    if(NOT IS_DIRECTORY ${test_dir})
        continue()
    endif()
    set(test_main "${test_dir}/test_main.cpp")
    if(NOT EXISTS ${test_main})
        continue()
    endif()
    get_filename_component(test_name ${test_dir} NAME)

    add_executable(${test_name}
        ${test_main}
        ${BTCLOCK_LIB_SOURCES}
        ${CMAKE_CURRENT_LIST_DIR}/unity_defaults.c
    )
    target_include_directories(${test_name} PRIVATE
        "${BTCLOCK_REPO_ROOT}/lib/btclock"
        "${BTCLOCK_REPO_ROOT}/main"
    )
    target_compile_definitions(${test_name} PRIVATE
        UNITY_TEST
        # NUM_SCREENS is referenced by several lib/btclock/ headers via
        # std::array<..., NUM_SCREENS>; the pin macros aren't exercised
        # by host tests but keep them defined so any guarded code still
        # compiles.
        MCP_INT_PIN=8
        NEOPIXEL_PIN=34
        NEOPIXEL_COUNT=4
        NUM_SCREENS=7
    )
    target_link_libraries(${test_name} PRIVATE unity)

    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
