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

