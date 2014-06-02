#!/bin/bash

#WALLET=1M72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd
SCRIPT=`dirname $0`/findminertags.sh

if [ ! -e ${SCRIPT} ] ; then
	echo Script ${SCRIPT} not found
	exit 1
fi

${SCRIPT} $1 $2 /etc/cgminer.conf
