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
