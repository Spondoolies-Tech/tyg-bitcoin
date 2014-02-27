#!/bin/sh

# host name prefix 
HOST_PREF="miner-"

# mng board eeprom range start for box serial name
START=84
END=95
EEP=/sys/bus/i2c/devices/0-0050/eeprom
# mng board eeprom range start for box serial name

#echo "${HOST_PREF=}`read-mng-eeprom-stripped.sh ${START} ${END}`"
echo "${HOST_PREF=}`rff ${EEP} ${START} ${END}`"
