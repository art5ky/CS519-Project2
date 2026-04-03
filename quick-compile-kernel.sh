#!/bin/bash

# Quick compile script for Linux kernel 5.15.0

KERN_PATH="$HOME/kernel_workspace/linux-5.15.0"
PROC=$(nproc)

cd "$KERN_PATH"

if [ ! -f .config ]; then
    echo "No .config found, copying from host..."
    cp /boot/config-$(uname -r) .config
fi

make olddefconfig

scripts/config --disable SYSTEM_TRUSTED_KEYS
scripts/config --disable SYSTEM_REVOCATION_KEYS

sudo make -j$PROC bzImage
sudo make modules_install
sudo make install
sudo update-grub

