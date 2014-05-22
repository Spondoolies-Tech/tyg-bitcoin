#!/bin/bash

usage()
{
	echo "Usage: $0 <script_file> <ip_addr_file> <extra> [xtr2 xtr3 ...]"
	echo "	if the script doesn't really require extra parm, just add a non-sense one"

}

if [ $# -lt 3 ] ; then
	usage
	exit 1
fi

IP_FILE=$2
SCRIPT_FILE=$1
SCRIPT_PARMS=${@:3}

if [ ! -e ${SCRIPT_FILE} ] ; then
	echo "Script file ${SCRIPT_FILE} not found."
	exit 2
fi

if [ ! -e ${IP_FILE} ] ; then
	echo "IP Addresses File ${IP_FILE} not found."
	exit 3
fi

MINERS=$(<${IP_FILE})

for MINER in ${MINERS}; do
#	echo "calling ${SCRIPT_FILE} ${MINER} ${SCRIPT_PARMS} "
	${SCRIPT_FILE} ${MINER} ${SCRIPT_PARMS} 	
done



