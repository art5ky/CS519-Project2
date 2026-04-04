#!/bin/bash -x
sudo apt update;

sudo sed -i 's/Types: deb/Types: deb deb-src/' /etc/apt/sources.list.d/ubuntu.sources
apt source linux-image-unsigned-$(uname -r)

cp -r linux-$(uname -r | cut -d'-' -f1) linux-$(uname -r | cut -d'-' -f1)-mod

sudo apt build-dep linux linux-image-unsigned-$(uname -r) -y
sudo apt install libncurses-dev gawk flex bison openssl libssl-dev dkms libelf-dev libudev-dev libpci-dev libiberty-dev autoconf llvm -y
sudo apt install git -y 
sudo apt-get install mosh

cd linux-$(uname -r | cut -d'-' -f1)-mod/
chmod a+x debian/rules
chmod a+x debian/scripts/*
chmod a+x debian/scripts/misc/*
fakeroot debian/rules clean
#fakeroot debian/rules editconfigs # you need to go through each (Y, Exit, Y, Exit..) or get a complaint about config later
fakeroot debian/rules clean
scripts/config --disable SYSTEM_TRUSTED_KEYS
scripts/config --disable SYSTEM_REVOCATION_KEYS