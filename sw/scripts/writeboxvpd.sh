#!/bin/sh
VPDSTR=$1

wtf /sys/bus/i2c/devices/0-0050/eeprom 84 "${VPDSTR:0:23}"
