#!/bin/bash

# For Ubuntu 22.04 LTS VM, install packages and source for linux kernel 5.15.0

SOURCES_FILE="/etc/apt/sources.list"
KERN_PATH="$HOME/kernel_workspace"

sudo sed -i 's/^#\s*deb-src/deb-src/' "$SOURCES_FILE"

sudo apt update
sudo apt build-dep -y linux
sudo apt install -y fakeroot llvm libncurses-dev dwarves libssl-dev libelf-dev binutils-dev

mkdir -p "$KERN_PATH"
cd "$KERN_PATH"
sudo chown "$USER":"$USER" "$KERN_PATH"

if [ ! -d "linux-5.15.0" ]; then
    apt source linux
fi

cd linux-5.15.*/
chmod a+x debian/rules
chmod a+x debian/scripts/*
chmod a+x debian/scripts/misc/*

#fakeroot debian/rules clean

if ! grep -q "5.15.0-999" debian.master/changelog; then
    sed -i '1s/5\.15\.0-[0-9]*/5.15.0-999/' debian.master/changelog
fi

export DEB_BUILD_OPTIONS="parallel=$(nproc)"
fakeroot debian/rules binary-headers binary-generic skipabi=true skipmodule=true

cd ..
sudo dpkg -i linux-headers-5.15.0-999*.deb \
             linux-image-unsigned-5.15.0-999*.deb \
             linux-modules-5.15.0-999*.deb
