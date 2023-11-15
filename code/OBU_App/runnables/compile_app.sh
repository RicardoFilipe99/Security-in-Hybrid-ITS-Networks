#!/bin/sh
cd ../
make CC=arm-linux-gnueabihf-gcc EXTRA_CFLAGS="-std=gnu99 -Wall -Wextra -march=armv7-a -mfloat-abi=hard -mfpu=vfpv3-d16 -mthumb"
