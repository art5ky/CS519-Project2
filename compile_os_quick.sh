#!/bin/bash -x
#sudo apt update; sudo apt-get install -y libdpkg-dev kernel-package libncurses-dev

PROC=`nproc`
export CONCURRENCY_LEVEL=$PROC
export CONCURRENCYLEVEL=$PROC

cd linux-5.15.0/

cp /boot/config-$(uname -r) .config
#make menuconfig
make oldconfig

scripts/config --disable SYSTEM_TRUSTED_KEYS
scripts/config --disable SYSTEM_REVOCATION_KEYS

touch REPORTING-BUGS
#sudo make clean -j
#sudo make prepare
sudo make -j$PROC
#sudo make modules -j$PROC
sudo make modules_install
sudo make install

sudo update-grub