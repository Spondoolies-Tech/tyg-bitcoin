#!/bin/bash
. $(dirname $0)/constants

IP_FILE=$1
OUT_PREF=$2
REPEATS=$3
DELAY=$4
if [ $# -lt 4 ] 
then
	echo usage $(fname) IP_FILE OUTPUT_PREFIX REPEATS DELAY
	exit
fi

for (( i=0; i<${REPEATS}; i++ ))
do 
	perform_scan.sh ${IP_FILE} ${SCANS_REPO}/${OUT_PREF}-$(($i+1))-$(date +%m%d%H%M).csv
	if [ $i -lt $((${REPEATS}-1)) ] ; then
		sleep ${DELAY}
	fi
done
