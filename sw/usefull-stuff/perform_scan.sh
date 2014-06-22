#!/bin/bash

usage()
{
	echo "Usage: $0 <ip_addr_file> "
}

if [ $# -gt 1 ] ; then
	usage
	exit 1
fi

SRC_IP_FILE=$1
SCRIPT_FILE=`dirname $0`/minerstat
SCRIPT_PARMS=${@:3}


if [ ! -e ~/scans ] ; then
	mkdir ~/scans
fi

OUT_FILE=~/scans/minerstats-`date +%m%d%H%M`.csv

echo "OUT_FILE ${OUT_FILE}"

if [ ! -e ${SCRIPT_FILE} ] ; then
	echo "Script file ${SCRIPT_FILE} not found."
	exit 2
fi

if [ ! -e ${SRC_IP_FILE} ] ; then
	echo "IP Addresses File ${IP_FILE} not found."
	exit 3
fi

IP_FILE=`mktemp`

cat $SRC_IP_FILE | cut -d ' ' -f 1 | cut -d , -f 1 > $IP_FILE

dos2unix $IP_FILE

CUNT=0

index=0
while read line ; do
	MINERS[$index]="${line}"
	index=$(($index+1))
done < ${IP_FILE}

OIFS="$IFS"
IFS=' '
read -a PARAMS <<< "${SCRIPT_PARMS}"
IFS="$OIFS"

#echo miners ${MINERS[@]} ${#MINERS[@]} 
#echo params ${PARAMS[@]} ${#PARAMS[@]}

for MINER in ${MINERS[@]}; do
	CUNT=$((${CUNT} + 1))
	${SCRIPT_FILE} ${MINER} ${CUNT}	| tee -a ${OUT_FILE}
done

echo "=============================================================="
echo "  Scan Completed                                              "
echo ".                                                             ."
echo "  Results reside in file: ${OUT_FILE}  in a  csv(excel) format"
echo "=============================================================="


rm ${IP_FILE}
