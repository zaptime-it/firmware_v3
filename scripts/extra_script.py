Import("env")

# Build web interface before building FS
def before_buildfs(source, target, env):
    env.Execute("cd data && yarn && yarn build")

env.AddPreAction("$BUILD_DIR/littlefs.bin", before_buildfs)
