#!/bin/bash -x

set -e

KERNEL_SRC="./linux-5.15.0"   # Kernel source dir
PATCH_DIR="./patches"          # Patch directory
PATCH_EXT="patch"              # Patch extension
FILES_DIR="./patches/root"     # Extra files to copy into kernel source

# Validate directories
if [ ! -d "$KERNEL_SRC" ]; then
    echo "Error: Kernel source directory '$KERNEL_SRC' not found"
    exit 1
fi

if [ ! -d "$PATCH_DIR" ]; then
    echo "Error: Patch directory '$PATCH_DIR' not found"
    exit 1
fi

# Copy extra files into kernel source (if files/ dir exists)
if [ -d "$FILES_DIR" ]; then
    echo "Copying files from '$FILES_DIR' into '$KERNEL_SRC'..."
    cp -rv "$FILES_DIR"/. "$KERNEL_SRC"/
    echo "Files copied successfully!"
fi

echo "Applying patches from '$PATCH_DIR' to '$KERNEL_SRC'..."

# Apply patches in sorted order
for patch in $(ls "$PATCH_DIR"/*."$PATCH_EXT" 2>/dev/null | sort); do
    echo "  Applying: $(basename $patch)"
    patch -d "$KERNEL_SRC" -p1 --forward < "$patch"
done

echo "All patches applied successfully!"