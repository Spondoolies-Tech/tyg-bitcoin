#!/bin/bash
IP_FILE=$1
OUT_PREF=$2
REPEATS=$3
DELAY=$4
if [ $# -lt 4 ] ; then
	echo usage $0 IP_FILE OUTPIUT_PREFIX REPEATS DELAY
	exit
fi
i=0
while [ $i -lt ${REPEATS} ]
do 
	~/miner_scripts/perform_scan.sh ${IP_FILE} ~/scans/${OUT_PREF}-`date +%m%d%H%M`.csv
	i=$(($i+1))
	sleep ${DELAY}
	
done
