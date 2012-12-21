#!/bin/bash
# Invoke from the sbus directory.

autoreconf -i

# NDK directory.
NDK=~/Downloads/android-ndk-r8b

# NDK Toolchain directory.
NDK_TOOLCHAIN=/tmp/android-toolchain

# Create the standalone toolchain.
$NDK/build/tools/make-standalone-toolchain.sh --platform=android-9 --install-dir=$NDK_TOOLCHAIN

# Add toolchain to path, and gcc g++ variables.
export PATH=$NDK_TOOLCHAIN/bin:$PATH
export CC=arm-linux-androideabi-gcc
export CXX=arm-linux-androideabi-g++

./configure --host=arm-linux-androideabi --target=arm-linux-androideabi

# http://code.google.com/p/android/issues/detail?id=35279
# cannot reference <limits> otherwise.
mv $NDK_TOOLCHAIN/arm-linux-androideabi/include/c++/4.6 $NDK_TOOLCHAIN/arm-linux-androideabi/include/c++/4.6.x-google

make

echo PATH=$PATH
echo CC=$CC
echo CXX=$CXX
