#!/bin/bash

#WALLET=1M72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd



if [ $# -lt 3 ] ; then
	echo "$0 <miner> <searchtag> <miner_file_to_grep>"
fi
LOOKATTRG=$3
TAG=$2
MINER=$1

#echo $@

ping -c 1 -W 1 ${MINER} > /dev/null 2>&1

if [ ! $? -eq 0 ] ; then
	echo ${MINER},NO-PING
	exit 11
fi

sshpass -p root ssh -o StrictHostKeyChecking=no root@${MINER} grep ${TAG} ${LOOKATTRG} > /dev/null 2>&1

if [ $? -eq 0 ] ; then
	
	STATUS=`sshpass -p root ssh -o StrictHostKeyChecking=no root@${MINER} cat /tmp/mg_status 2>/dev/null`
	RATE_TEMP=`sshpass -p root ssh -o StrictHostKeyChecking=no root@${MINER} cat /tmp/mg_rate_temp 2>/dev/null`
	echo ${MINER},${TAG},${STATUS},${RATE_TEMP}
fi 

