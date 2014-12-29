#!/bin/sh
MINESTAT=`cat /mnt/config/etc/mining_status`
/usr/local/bin/spond-manager stop
mkdir /tmp/etcbu
mkdir /tmp/etcbu/etc
mkdir /tmp/etcbu/rrd
cp /mnt/config/rrd/* /tmp/etcbu/rrd/
cp /mnt/config/etc/cgminer.conf /tmp/etcbu/etc/
cp /mnt/config/etc/minepeon.conf /tmp/etcbu/etc/
cp /mnt/config/etc/mining_status /tmp/etcbu/etc/
cp /mnt/config/etc/mg_* /tmp/etcbu/etc/

rm -rf /mnt/config/*
mkdir /mnt/config/log
mkdir /mnt/config/rrd
mkdir /mnt/config/etc

cp /tmp/etcbu/rrd/* /mnt/config/rrd
cp /tmp/etcbu/etc/* /mnt/config/etc
echo ${MINESTAT} > /mnt/config/etc/mining_status
sync
/sbin/reboot
