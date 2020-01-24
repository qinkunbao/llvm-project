#!/bin/sh

for i in $(grep -l /apex/com.android.runtime/lib64/bionic/libc.so /proc/*/maps | grep -o '[0-9][0-9]*'); do
  echo $i
  /data/local/tmp/dump_depot_log $i /data/local/tmp/logs/$i-$(cat /proc/$i/comm).log
done
