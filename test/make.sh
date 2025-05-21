#!/bin/bash

PWD_KEYSTONE_DIR=$(pwd)
export PATH=$PATH:${PWD_KEYSTONE_DIR}/build-starfive/visionfive264/buildroot.build/host/bin
export EXAMPLES_DIR_NAME=$(ls build-starfive/visionfive264/buildroot.build/build | grep keystone-examples)

echo "Examples dir name: $EXAMPLES_DIR_NAME"

./test/os_access_epm/make.sh

./test/os_access_stm/make.sh

./test/other_enclave_access_stm/make.sh

./test/other_os_access_epm/make.sh

./test/other_os_access_stm/make.sh

./test/other_enclave_access_epm/make.sh

ls test/*/loader.bin -l
ls test/*/eyrie-rt -l
ls test/*/*_eapp* -l
ls test/*/*_host* -l
