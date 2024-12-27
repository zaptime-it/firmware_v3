Import("env")
import os
import gzip
from shutil import copyfileobj, rmtree, copyfile, copytree
from pathlib import Path
import subprocess




revision = (
    subprocess.check_output(["git", "rev-parse", "HEAD"])
    .strip()
    .decode("utf-8")
)

def gzip_file(input_file, output_file):
    with open(input_file, 'rb') as f_in:
        with gzip.open(output_file, 'wb') as f_out:
            copyfileobj(f_in, f_out)

def process_directory(input_dir, output_dir):
    if os.path.exists(output_dir):
        rmtree(output_dir)
    for root, dirs, files in os.walk(input_dir):
        relative_path = os.path.relpath(root, input_dir)
        output_root = os.path.join(output_dir, relative_path)

        Path(output_root).mkdir(parents=True, exist_ok=True)

        for file in files:
#            if not file.endswith(('.bin')):
                input_file_path = os.path.join(root, file)
                output_file_path = os.path.join(output_root, file + '.gz')
                gzip_file(input_file_path, output_file_path)
                print(f'Compressed: {input_file_path} -> {output_file_path}')
    file_path = os.path.join(output_dir, "fs_hash.txt")
    with open(file_path, "w") as file:
        file.write(revision)


# Build web interface before building FS
def before_buildfs(source, target, env):

    env.Execute("cd data && yarn && yarn postinstall && yarn build")
    input_directory = 'data/dist'
    output_directory = 'data/build_gz'
#    copytree("assets", "data/dist/assets")

    process_directory(input_directory, output_directory)

def get_fs_partition_size(env):
    import csv
    
    # Get partition table path - first try custom, then default
    board_config = env.BoardConfig()
    partition_table = board_config.get("build.partitions", "default.csv")
    
    # Handle default partition table path
    if partition_table == "default.csv" or partition_table == "huge_app.csv":
        partition_table = os.path.join(env.PioPlatform().get_package_dir("framework-arduinoespressif32"), 
                                     "tools", "partitions", partition_table)
    
    # Parse CSV to find spiffs/littlefs partition
    with open(partition_table, 'r') as f:
        for row in csv.reader(f):
            if len(row) < 5:
                continue
            # Remove comments and whitespace
            row = [cell.strip().split('#')[0] for cell in row]
            # Check if this is a spiffs or littlefs partition
            if row[0].startswith(('spiffs', 'littlefs')):
                # Size is in hex format
                return int(row[4], 16)
    return 0

def get_littlefs_used_size(binary_path):  
    mklittlefs_path = os.path.join(env.PioPlatform().get_package_dir("tool-mklittlefs-rp2040-earlephilhower"), "mklittlefs")

    try:
        result = subprocess.run([mklittlefs_path, '-l', binary_path], capture_output=True, text=True)
                
        if result.returncode == 0:
            # Parse the output to sum up file sizes
            total_size = 0
            for line in result.stdout.splitlines():
                if line.strip() and not line.startswith('<dir>') and not line.startswith('Creation'): 
                    # Each line format: size filename
                    size = line.split()[0]
                    total_size += int(size)
            return total_size
    except Exception as e:
        print(f"Error getting filesystem size: {e}")
    return 0

 
def after_littlefs(source, target, env):
    binary_path = str(target[0])
    partition_size = get_fs_partition_size(env)
    used_size = get_littlefs_used_size(binary_path)
    
    percentage = (used_size / partition_size) * 100
    bar_width = 50
    filled = int(bar_width * percentage / 100)
    bar = '=' * filled + '-' * (bar_width - filled)
    
    print(f"\nLittleFS Actual Usage: [{bar}] {percentage:.1f}% ({used_size}/{partition_size} bytes)")


flash_size = env.BoardConfig().get("upload.flash_size", "4MB")
fs_image_name = f"littlefs_{flash_size}"
env.Replace(ESP32_FS_IMAGE_NAME=fs_image_name)
env.Replace(ESP8266_FS_IMAGE_NAME=fs_image_name)

os.environ["PUBLIC_BASE_URL"] = ""
fs_name = env.get("ESP32_FS_IMAGE_NAME", "littlefs.bin")
# Or alternatively:
# fs_name = env.get("FSTOOLNAME", "littlefs.bin")

# Use the variable in the pre-action
env.AddPreAction(f"$BUILD_DIR/{fs_name}.bin", before_buildfs)
env.AddPostAction(f"$BUILD_DIR/{fs_name}.bin", after_littlefs)
# LittleFS Actual Usage: [==============================--------------------] 60.4% (254165/420864 bytes)
# LittleFS Actual Usage: [==============================--------------------] 60.2% (253476/420864 bytes)
# 372736 used