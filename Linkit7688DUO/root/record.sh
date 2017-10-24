#!/bin/sh -e

rm -f /mnt/sdcard/record-test.wav

arecord -f cd -D plughw:1,0 -t wav -M /mnt/sdcard/record-test.wav &

sleep 180

killall arecord

