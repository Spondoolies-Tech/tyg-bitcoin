#!/bin/sh

# Written by Vladik Goytin

EEPROM=''
EEPROM=${EEPROM}'\xaa\x55\x33\xee\x41\x33\x33\x35\x42\x4e\x4c\x54\x30\x41\x35\x42'
EEPROM=${EEPROM}'\x32\x33\x31\x33\x42\x42\x42\x4b\x33\x36\x38\x32'

main()
{
	echo 'Writing default EEPROM'
	echo -n -e ${EEPROM} > /sys/bus/i2c/devices/0-0050/eeprom
}

main $@
