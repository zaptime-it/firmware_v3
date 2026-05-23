import os

Import("env")

flash_size = env.BoardConfig().get("upload.flash_size", "4MB")
fs_image_name = f"littlefs_{flash_size}"
env.Replace(ESP32_FS_IMAGE_NAME=fs_image_name)
env.Replace(ESP8266_FS_IMAGE_NAME=fs_image_name)


def _guard_websockets_max_data_size(env):
    """Make WEBSOCKETS_MAX_DATA_SIZE overridable via a build flag.

    The Links2004 WebSockets library hard-codes a 15 KB cap on incoming
    frames (see WebSockets.h, unconditional `#define`). mempool.space
    ships an initial 18 KB `blocks` history burst immediately after our
    subscription; the library rejects it with close code 1009, which
    manifests as an endless "Mempool.space WS Connection Closed" loop.

    Rather than fork the library, we wrap every occurrence of its
    hard-coded define in an `#ifndef` so a build_flag
    `-D WEBSOCKETS_MAX_DATA_SIZE=32768` wins. Idempotent: subsequent
    builds detect the guard and skip the rewrite.
    """
    project_dir = env["PROJECT_DIR"]
    libdeps_dir = env.subst("$PROJECT_LIBDEPS_DIR")
    piobuildenv = env["PIOENV"]
    header = os.path.join(libdeps_dir, piobuildenv, "WebSockets", "src", "WebSockets.h")
    if not os.path.isfile(header):
        return

    SENTINEL = "// patched by pre_script.py to allow build-flag override"
    with open(header, "r") as f:
        src = f.read()
    if SENTINEL in src:
        return

    new_src = src.replace(
        "#define WEBSOCKETS_MAX_DATA_SIZE (15 * 1024)",
        SENTINEL
        + "\n#ifndef WEBSOCKETS_MAX_DATA_SIZE\n"
        + "#define WEBSOCKETS_MAX_DATA_SIZE (15 * 1024)\n"
        + "#endif",
    )
    if new_src == src:
        return
    with open(header, "w") as f:
        f.write(new_src)
    print(
        f"[pre_script] patched {os.path.relpath(header, project_dir)} "
        f"to honour -D WEBSOCKETS_MAX_DATA_SIZE"
    )


_guard_websockets_max_data_size(env)



def _force_websockets_rx_internal(env):
    """Force the Links2004 WebSockets RX frame buffer into internal DRAM.

    The library allocates the incoming frame buffer with a plain
    `malloc(header->payloadLen + 1)` in WebSockets.cpp (the
    handleWebsocket() payload branch). With PSRAM available and a
    lowered SPIRAM_MALLOC_ALWAYSINTERNAL threshold, the kernel may hand
    us SPIRAM for allocations of this size — a mempool.space `blocks`
    frame is ~18 KB, well above the 4 KB threshold we want to use so
    that the 16 KB mbedtls IN buffer lands in PSRAM. PSRAM-backed WS
    receive buffers break here because the path from the Wi-Fi/LWIP RX
    stack to the TLS decrypt code is not reliably tolerant of
    non-DMA-capable memory at this boundary, and manifests as mempool
    WS connecting but never delivering frames.

    Force the one allocation that must stay in internal DRAM to use
    `heap_caps_malloc(size, MALLOC_CAP_INTERNAL)`. Idempotent via
    sentinel comment.
    """
    project_dir = env["PROJECT_DIR"]
    libdeps_dir = env.subst("$PROJECT_LIBDEPS_DIR")
    piobuildenv = env["PIOENV"]
    cpp = os.path.join(libdeps_dir, piobuildenv, "WebSockets", "src", "WebSockets.cpp")
    if not os.path.isfile(cpp):
        return

    SENTINEL = "// patched by pre_script.py to force RX buffer to internal DRAM (v2)"
    with open(cpp, "r") as f:
        src = f.read()
    if SENTINEL in src:
        return

    target = "payload = (uint8_t *)malloc(header->payloadLen + 1);"
    replacement = (
        SENTINEL + "\n"
        "#if defined(ESP32) && __has_include(<esp_heap_caps.h>)\n"
        "        payload = (uint8_t *)heap_caps_malloc(header->payloadLen + 1, MALLOC_CAP_INTERNAL);\n"
        "        if (!payload) {\n"
        "            // DRAM too fragmented for an 18 KB contiguous chunk right\n"
        "            // now; fall back to the default heap (which may return\n"
        "            // PSRAM). That's slower and not DMA-capable but keeps the\n"
        "            // WS session alive instead of tearing it down mid-frame.\n"
        "            payload = (uint8_t *)heap_caps_malloc(header->payloadLen + 1, MALLOC_CAP_DEFAULT);\n"
        "        }\n"
        "#else\n"
        "        payload = (uint8_t *)malloc(header->payloadLen + 1);\n"
        "#endif"
    )
    new_src = src.replace(target, replacement)
    if new_src == src:
        return

    # esp_heap_caps.h needs to be available where the replacement runs.
    # Inject the include near the top if not already present.
    if "#include <esp_heap_caps.h>" not in new_src:
        new_src = new_src.replace(
            "#include \"WebSockets.h\"",
            "#include \"WebSockets.h\"\n"
            "#if defined(ESP32) && __has_include(<esp_heap_caps.h>)\n"
            "#include <esp_heap_caps.h>\n"
            "#endif",
            1,
        )

    with open(cpp, "w") as f:
        f.write(new_src)
    print(
        f"[pre_script] patched {os.path.relpath(cpp, project_dir)} "
        f"to force WebSockets RX buffer into internal DRAM"
    )


_force_websockets_rx_internal(env)


def _guard_ubitcoin_esp_random(env):
    """Make uBitcoin's trezor/rand.c compile against ESP-IDF v5.x.

    `esp_random()` used to be declared in <esp_system.h> on IDF v4.x;
    v5.x moved it to <esp_random.h> and <esp_system.h> no longer brings
    it in transitively. uBitcoin (pulled in via nostrduino) still only
    includes <esp_system.h>, producing
    `implicit declaration of function 'esp_random'`.

    Idempotent: sentinel-guarded.
    """
    import glob

    project_dir = env["PROJECT_DIR"]
    libdeps_dir = env.subst("$PROJECT_LIBDEPS_DIR")
    piobuildenv = env["PIOENV"]
    env_libdeps = os.path.join(libdeps_dir, piobuildenv)
    if not os.path.isdir(env_libdeps):
        return

    # uBitcoin lives at top-level for some envs (lolin_s3_mini, btclock_rev_b)
    # but nested under Nostrduino/src/ for others (lolin_s3_mini_*_epd,
    # btclock_v8_213epd). Glob covers both layouts.
    candidates = glob.glob(
        os.path.join(env_libdeps, "**", "trezor", "rand.c"),
        recursive=True,
    )

    SENTINEL = "/* patched by pre_script.py for IDF v5 esp_random move */"
    target = "  #include <esp_system.h>"
    replacement = (
        SENTINEL + "\n"
        "  #include <esp_system.h>\n"
        "  #include <esp_random.h>"
    )
    for src_path in candidates:
        with open(src_path, "r") as f:
            src = f.read()
        if SENTINEL in src:
            continue
        new_src = src.replace(target, replacement, 1)
        if new_src == src:
            continue
        with open(src_path, "w") as f:
            f.write(new_src)
        print(
            f"[pre_script] patched {os.path.relpath(src_path, project_dir)} "
            f"to include <esp_random.h> for IDF v5"
        )


_guard_ubitcoin_esp_random(env)


def _guard_gxepd2_for_modern_gcc(env):
    """Patch the dsbaars/GxEPD2#universal_pin fork for GCC 11+.

    The fork replaces `int` pin members with `UniversalPin*` but missed
    several `if (_rst >= 0)` style checks where the operand is now a
    pointer. Newer GCC (shipped with IDF v5.5 toolchain) rejects that
    ordered comparison of pointer with integer zero as an error, not a
    warning. Replace the bare `>= 0` checks with `!= nullptr` -- the
    fork's intent is "is a pin assigned?", which on the pointer model
    means non-null.

    Also injects `#include <stdexcept>` into GxEPD2_EPD.cpp where
    `std::runtime_error` is used but never included (worked transitively
    on older libstdc++ headers).

    Idempotent per-file via sentinel marker at top.
    """
    import re

    project_dir = env["PROJECT_DIR"]
    libdeps_dir = env.subst("$PROJECT_LIBDEPS_DIR")
    piobuildenv = env["PIOENV"]
    root = os.path.join(libdeps_dir, piobuildenv, "GxEPD2", "src")
    if not os.path.isdir(root):
        return

    SENTINEL = "// patched by pre_script.py: GxEPD2 fork pin>=0 -> != nullptr"
    PIN_RE = re.compile(r"\bif \((_(rst|dc|cs|busy))\s*>=\s*0\)")

    patched = 0
    for dirpath, _dirnames, filenames in os.walk(root):
        for fname in filenames:
            if not fname.endswith((".cpp", ".h")):
                continue
            fpath = os.path.join(dirpath, fname)
            with open(fpath, "r") as f:
                src = f.read()
            if SENTINEL in src:
                continue
            new_src = PIN_RE.sub(r"if (\1 != nullptr)", src)
            if fname == "GxEPD2_EPD.cpp" and "<stdexcept>" not in new_src:
                # std::runtime_error used at line ~163 but never included.
                new_src = new_src.replace(
                    "#include \"GxEPD2_EPD.h\"",
                    "#include \"GxEPD2_EPD.h\"\n#include <stdexcept>",
                    1,
                )
            if new_src == src:
                continue
            new_src = SENTINEL + "\n" + new_src
            with open(fpath, "w") as f:
                f.write(new_src)
            patched += 1

    if patched:
        print(
            f"[pre_script] patched {patched} GxEPD2 file(s) for modern GCC "
            f"(pin pointer-vs-int + std::runtime_error)"
        )


_guard_gxepd2_for_modern_gcc(env)
