#!/bin/sh
/usr/local/bin/spond-manager stop
mkdir /tmp/etcbu
mkdir /tmp/etcbu/etc
cp /mnt/config/etc/mg_* /tmp/etcbu/etc

rm -rf /mnt/mmc-config/*
mkdir /mnt/mmc-config/etc

cp /tmp/etcbu/etc/* /mnt/mmc-config/etc
sync
/sbin/reboot
