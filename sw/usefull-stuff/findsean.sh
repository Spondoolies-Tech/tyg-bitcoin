#!/bin/bash

SEAN=1M72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd

MINER=$1

ping -c 1 -W 1 ${MINER} > /dev/null 2>&1

if [ ! $? -eq 0 ] ; then
exit 11
fi

sshpass -p root ssh root@${MINER} grep ${SEAN} /etc/cgminer.conf > /dev/null 2>&1

if [ $? -eq 0 ] ; then
	echo ${MINER}
fi 

