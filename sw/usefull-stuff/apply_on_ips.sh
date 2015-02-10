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
SCRIPT_PARMS=(${@:3})

if [ ! -e ${SCRIPT_FILE} ] && [ ! -e `which ${SCRIPT_FILE}` ] ; then
	echo "Script file ${SCRIPT_FILE} not found."
	exit 2
fi

if [ ! -e ${SRC_IP_FILE} ] ; then
	echo "IP Addresses File ${IP_FILE} not found."
	exit 3
fi

IP_FILE=`mktemp`

cat $SRC_IP_FILE | cut -d ' ' -f 1 | cut -d , -f 1 | sed -e 's/\([^#]*\)#.*/\1/g' > $IP_FILE

dos2unix $IP_FILE

MINERS=($(cat $IP_FILE))

for MINER in ${MINERS[@]}; do
	${SCRIPT_FILE} ${MINER} ${SCRIPT_PARMS[@]}	
done

rm ${IP_FILE}
