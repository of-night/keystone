#!/bin/bash

sync
echo "clean cache start"
echo 1 > /proc/sys/vm/drop_caches
echo 2 > /proc/sys/vm/drop_caches
echo 3 > /proc/sys/vm/drop_caches
sync
echo "clean cache done"

