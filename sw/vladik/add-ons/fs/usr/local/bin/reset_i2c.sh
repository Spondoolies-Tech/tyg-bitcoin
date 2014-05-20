#!/bin/sh

GP=111

if [ ! -e /sys/class/gpio/gpio${GP}/ ] ; then
	echo ${GP} > /sys/class/gpio/export
fi

echo out > /sys/class/gpio/gpio${GP}/direction
echo 0 > /sys/class/gpio/gpio${GP}/value
usleep 50000

echo 1 > /sys/class/gpio/gpio${GP}/value
usleep 50000

RC=0

I2C_SW=`i2cdetect -y -r 0 | grep  70 | cut -d ' ' -f 2`

if [ "${I2C_SW}" == "70" ] ; then
	echo "I2C Switch OK"
elif [ "${I2C_SW}" == "--" ] ; then
	echo "I2C Switch BAD !!"
	RC=1
fi


exit ${RC}
