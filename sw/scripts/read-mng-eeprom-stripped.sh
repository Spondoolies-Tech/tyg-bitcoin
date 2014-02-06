#!/bin/sh

FN="/sys/bus/i2c/devices/0-0050/eeprom"
let offs=0
let start=$1
let end=$2-1
VALIDCHAR="1234567890-abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
VPDI=""


while [  $offs -le $end ]   
do

  IFS= read -n 1 ch 
  if [ $offs -ge $start ] ; then
	if	[ `echo ${VALIDCHAR} | grep -c "${ch}" ` -eq 1 ]; then
		  VPDI="${VPDI}${ch}"
	fi
  fi
  let offs++
done < "${FN}"

echo "${VPDI}"
