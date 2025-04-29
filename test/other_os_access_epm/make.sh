#!/bin/bash

# eapp
riscv64-buildroot-linux-gnu-gcc -static -o ./test/other_os_access_epm/other_os_access_epm_eapp \
	./test/other_os_access_epm/eapp/other_os_access_epm_eapp.c \
	-L/home/yx/Desktop/vf-keystone/keystone/build-starfive/visionfive264/buildroot.build/per-package/host-keystone-sdk/host/usr/share/keystone/sdk/lib \
	-lkeystone-edge \
	-lkeystone-eapp \
	-I/home/yx/Desktop/vf-keystone/keystone/build-starfive/visionfive264/buildroot.build/per-package/host-keystone-sdk/host/usr/share/keystone/sdk/include/edge \
	-I/home/yx/Desktop/vf-keystone/keystone/build-starfive/visionfive264/buildroot.build/per-package/host-keystone-sdk/host/usr/share/keystone/sdk/include
	
# host
riscv64-buildroot-linux-gnu-g++ -o ./test/other_os_access_epm/other_os_access_epm_host \
	./test/other_os_access_epm/host/other_os_access_epm_host.cpp \
	-L./build-starfive/visionfive264/buildroot.build/per-package/host-keystone-sdk/host/usr/share/keystone/sdk/lib \
	-lkeystone-host \
	-lkeystone-edge \
	-I./build-starfive/visionfive264/buildroot.build/per-package/host-keystone-sdk/host/usr/share/keystone/sdk/include/host \
	-I./build-starfive/visionfive264/buildroot.build/per-package/host-keystone-sdk/host/usr/share/keystone/sdk/include/edge \
	-I./build-starfive/visionfive264/buildroot.build/per-package/host-keystone-sdk/host/usr/share/keystone/sdk/include \
	-static

# eyrie-rt
cp build-starfive/visionfive264/buildroot.build/build/${EXAMPLES_DIR_NAME}/hello/eyrie-rt ./test/other_os_access_epm/

# loader.bin
cp build-starfive/visionfive264/buildroot.build/build/${EXAMPLES_DIR_NAME}/hello/loader.bin ./test/other_os_access_epm/
