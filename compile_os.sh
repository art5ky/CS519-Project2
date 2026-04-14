#!/bin/bash -x
set -euo pipefail

sudo apt update
sudo apt-get install -y libncurses-dev bison flex

PROC=$(nproc)
export CONCURRENCY_LEVEL=$PROC
export CONCURRENCYLEVEL=$PROC

cd linux-5.15.0/

cp /boot/config-$(uname -r) .config
make oldconfig

scripts/config --set-str SYSTEM_TRUSTED_KEYS ""
scripts/config --set-str SYSTEM_REVOCATION_KEYS ""

touch REPORTING-BUGS

make clean
make prepare
make -j"$PROC"
make modules -j"$PROC"

# Sanity check: only continue if key build artifacts exist
ls -l vmlinux System.map arch/x86/boot/bzImage

sudo make modules_install
sudo make install
sudo update-grub