#!/bin/bash
. $(dirname $0)/constants




if [ $# -lt 3 ] ; then
	echo "Usage: $(fname $0) <miner> <searchtag> <miner_file_to_grep>"
fi
LOOKATTRG=$3
TAG=$2
MINER=$1

ping -c 1 -W 1 ${MINER} > /dev/null 2>&1

if [ ! $? -eq 0 ] ; then
	echo ${MINER},NO-PING
	exit 11
fi

${SSHPASS_CMD} ${DEFAULT_USER}@${MINER} grep ${TAG} ${LOOKATTRG} > /dev/null 2>&1

if [ $? -eq 0 ] ; then
	
	STATUS=`${SSHPASS_CMD} ${DEFAULT_USER}@${MINER} cat /tmp/mg_status 2>/dev/null`
	RATE_TEMP=`${SSHPASS_CMD} ${DEFAULT_USER}@${MINER} cat /tmp/mg_rate_temp 2>/dev/null`
	echo ${MINER},${TAG},${STATUS},${RATE_TEMP}
fi 

