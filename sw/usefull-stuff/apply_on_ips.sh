#!/bin/bash

usage()
{
	echo "Usage: $0 <script_file> <ip_addr_file> params"
	echo "	if the script doesn't really require extra parm, just add a non-sense one"

}

if [ $# -lt 3 ] ; then
	usage
	exit 1
fi

SRC_IP_FILE=$2
SCRIPT_FILE=$1
#OPERRATIVE=$3
#FILTER=$4
SCRIPT_PARMS=${@:3}

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
#	echo "calling ${SCRIPT_FILE} ${MINER} ${PARAMS[@]}" 
	${SCRIPT_FILE} ${MINER} ${PARAMS[@]}	
done

rm ${IP_FILE}
