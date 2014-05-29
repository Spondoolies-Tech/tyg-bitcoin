#!/bin/sh

GP=110

CMD=1

echo $1
if [ "$1" = "ON" ]; then
	CMD=0
fi

if [ "$1" = "OFF" ]; then
	CMD=1
fi

echo CMD = ${CMD}

if [ ! -e /sys/class/gpio/gpio${GP}/ ] ; then
	echo ${GP} > /sys/class/gpio/export
fi

echo out > /sys/class/gpio/gpio${GP}/direction
echo ${CMD} > /sys/class/gpio/gpio${GP}/value

RC=$?

usleep 50000

exit ${RC}
