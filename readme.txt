# ---- INSTRUCTIONS ---- 


# 1. Use installation script

./intall_packages.sh

# 2. Compile the kernel

./compile_os.sh

# 3. Create a copy of the linux source code for future patching

cp -a linux-5.15.0/ linux-5.15.0-orig/


# ---- HELPFUL COMMANDS ---- 


# Create a patch file using linux-5.15.0 and linux-5.15.0-orig

diff -u linux-5.15.0-orig/ linux-5.15.0/ > patches/kernel-file.patch

# Subsequent changes, use the quick compilation script

./compile_os_quick.sh

