#!/bin/sh
date -s 2015-01-01
export TZ=GMT-8
hwclock -t
ulimit -s 512

ifconfig lo up
ifconfig wlan0 up

mkdir -p /var/log
mkdir -p /var/run

cd /usr/bin;
insmod board_ctl.ko

#turn on red flicker
killall ledCtrl;
ledCtrl -r &

#statr main
cd /usr/bin;
./sys_daemon &



