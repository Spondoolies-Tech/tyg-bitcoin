#!/bin/sh
VPDSTR=$1

########123456789012345678901234567890123456789012345678901234567890123466
SPACES="                                                                  "
./eeprom-provisioning.sh
./wtf /sys/bus/i2c/devices/0-0050/eeprom 5 "${SPACES:0:66}"
./wtf /sys/bus/i2c/devices/0-0050/eeprom 5 "${VPDSTR:0:23}"
