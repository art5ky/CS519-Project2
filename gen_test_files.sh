#!/bin/bash -x

# File sizes are 4MB, 8MB, 16MB, 32MB, and 64MB. This can be altered.
FILE_SIZES=(4 8 16 32 64)

# Directory to store the test files
TARGET_DIR="./test_files"

# Create the directory if it doesn't exist
mkdir -p "$TARGET_DIR"

echo "--- Generating Test Files ---"

for SIZE in "${FILE_SIZES[@]}"
do
    FILE_NAME="${TARGET_DIR}/testfile_${SIZE}MB.bin"
    
    echo "[*] Creating $FILE_NAME ($SIZE MB)..."
    
    dd if=/dev/zero of="$FILE_NAME" bs=1M count="$SIZE"
    
    echo "Finished $FILE_NAME"
    echo "--------------------------------"
done

echo "All files generated in $TARGET_DIR"
ls -lh "$TARGET_DIR"