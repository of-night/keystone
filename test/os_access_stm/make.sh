#!/bin/bash

# eapp
riscv64-buildroot-linux-gnu-gcc -static -o ./test/os_access_stm/os_access_stm_eapp \
	./test/os_access_stm/eapp/os_access_stm_eapp.c \
	-L/home/yx/Desktop/vf-keystone/keystone/build-starfive/visionfive264/buildroot.build/per-package/host-keystone-sdk/host/usr/share/keystone/sdk/lib \
	-lkeystone-edge \
	-lkeystone-eapp \
	-I/home/yx/Desktop/vf-keystone/keystone/build-starfive/visionfive264/buildroot.build/per-package/host-keystone-sdk/host/usr/share/keystone/sdk/include/edge \
	-I/home/yx/Desktop/vf-keystone/keystone/build-starfive/visionfive264/buildroot.build/per-package/host-keystone-sdk/host/usr/share/keystone/sdk/include
	
# host
riscv64-buildroot-linux-gnu-g++ -o ./test/os_access_stm/os_access_stm_host \
	./test/os_access_stm/host/os_access_stm_host.cpp \
	-L/home/yx/Desktop/vf-keystone/keystone/build-starfive/visionfive264/buildroot.build/per-package/host-keystone-sdk/host/usr/share/keystone/sdk/lib \
	-lkeystone-host \
	-lkeystone-edge \
	-I/home/yx/Desktop/vf-keystone/keystone/build-starfive/visionfive264/buildroot.build/per-package/host-keystone-sdk/host/usr/share/keystone/sdk/include/host \
	-I/home/yx/Desktop/vf-keystone/keystone/build-starfive/visionfive264/buildroot.build/per-package/host-keystone-sdk/host/usr/share/keystone/sdk/include/edge \
	-I/home/yx/Desktop/vf-keystone/keystone/build-starfive/visionfive264/buildroot.build/per-package/host-keystone-sdk/host/usr/share/keystone/sdk/include \
	-static

# eyrie-rt
cp build-starfive/visionfive264/buildroot.build/build/keystone-examples-d494a0e382852bce/hello/eyrie-rt ./test/os_access_stm/

# loader.bin
cp build-starfive/visionfive264/buildroot.build/build/keystone-examples-d494a0e382852bce/hello/loader.bin ./test/os_access_stm/

