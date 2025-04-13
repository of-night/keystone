#!/bin/bash

date
sudo dd if=build-starfive/visionfive264/buildroot.build/images/sdcard.img of=/dev/sdb bs=4M conv=fdatasync status=progress
date
