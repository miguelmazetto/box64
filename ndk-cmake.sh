#!/bin/bash
SDIR=$PWD
mkdir build; cd build
export CFLAGS="$CFLAGS -Wimplicit-function-declaration"
cmake -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
      -DANDROID_PLATFORM=24\
      -DANDROID_ABI=arm64-v8a ..