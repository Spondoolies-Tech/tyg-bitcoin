#!/bin/sh

# host name prefix 
HOST_PREF="SPOND-"

# mng board eeprom range start for box serial name
START=28

# mng board eeprom range start for box serial name
END=39
echo "${HOST_PREF=}`./read-mng-eeprom-stripped.sh 28 39`"
