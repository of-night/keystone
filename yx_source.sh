#!/bin/bash

export KEYSTONE_PLATFORM=starfive/visionfive2
export KEYSTONE_BITS=64
export BUILDROOT_CONFIGFILE=riscv64_starfive_visionfive2_defconfig
export BUILDROOT_TARGET=all

cd demo
source source.sh
cd ..
