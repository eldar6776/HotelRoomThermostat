import datetime
import os
import re

try:
    Import("env")
except NameError:
    # Running outside PlatformIO SCons build system
    pass

FILENAME_VERSION = "include/version.h"
BUILD_NUMBER_FILE = "build_no.txt"

build_no = None

# 1. Attempt to read from build_no.txt
if os.path.exists(BUILD_NUMBER_FILE):
    try:
        with open(BUILD_NUMBER_FILE, 'r') as f:
            build_no = int(f.read().strip()) + 1
    except ValueError:
        pass

# 2. If build_no.txt wasn't found or was invalid, try to parse current version.h
if build_no is None and os.path.exists(FILENAME_VERSION):
    try:
        with open(FILENAME_VERSION, 'r') as f:
            content = f.read()
            match = re.search(r'#define\s+VERSION_PATCH\s+(\d+)', content)
            if match:
                build_no = int(match.group(1)) + 1
    except Exception:
        pass

# 3. Fallback to 1 if everything else failed
if build_no is None:
    build_no = 1

# Write new build number back to build_no.txt
with open(BUILD_NUMBER_FILE, 'w') as f:
    f.write(str(build_no))

# Generate the C header content
header_content = f"""#ifndef VERSION_H
#define VERSION_H

#define VERSION_MAJOR 1
#define VERSION_MINOR 0
#define VERSION_PATCH {build_no}
#define BUILD_DATE "{datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")}"

#endif
"""

# Ensure the include/ directory exists
os.makedirs(os.path.dirname(FILENAME_VERSION), exist_ok=True)

# Write out the version.h file
with open(FILENAME_VERSION, 'w') as f:
    f.write(header_content)

print(f"[VERSION] Incremented build/patch number to: {build_no}")