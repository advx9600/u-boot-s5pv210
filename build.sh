#!/bin/bash
#source ../android4.0_realv210_ver_1_0/build/envsetup.sh
make distclean
make smdkv210single_config
make CROSS_COMPILE=arm-linux- -j4
