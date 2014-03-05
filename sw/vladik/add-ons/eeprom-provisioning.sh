#!/bin/sh

EEPROM=''
#0 - 3 BBB Magic
EEPROM=${EEPROM}'\xaa\x55\x33\xee'

#4 - 11 BBB Name
EEPROM=${EEPROM}'\x41\x33\x33\x35\x42\x4e\x4c\x54'

#12 - 15 BBB Name
EEPROM=${EEPROM}'\x30\x41\x35\x42'

#16 - 27 BBB Serial
EEPROM=${EEPROM}'\x32\x33\x31\x33\x42\x42\x42\x4b\x33\x36\x38\x32'

#28 - 59 BBB Config block
EEPROM=${EEPROM}'\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff'
EEPROM=${EEPROM}'\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff'

#60 (Miner's) VPD REV
EEPROM=${EEPROM}'\x01'


main()
{
	echo 'Writing default EEPROM'
	echo -n -e ${EEPROM} > /sys/bus/i2c/devices/0-0050/eeprom
}

main $@
