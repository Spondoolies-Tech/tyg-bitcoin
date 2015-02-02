#!/bin/bash
. $(dirname $0)/constants

if [ $# -lt 2 ] ; then
	echo "Too few parameters !"
	echo "Usage: $0 <IP> <scriptfile>"
	echo "       IP 	- The miner's IP address"
	echo "       scriptfile - the script to be pushed onto miner and called"
	exit 1
fi
IP=$1
SCR_LOCAL_REL_NAME=$2
SCR_FILE_NAME=`echo ${SCR_LOCAL_REL_NAME} | sed -e 's/.*\/\(.*\)/\1/g'`
TARGET_DIR=/usr/local/bin
SCR_TARGET_NAME=${TARGET_DIR}/${SCR_FILE_NAME}
ping -c 1 -W 1 $IP > /dev/null 2>&1
if [ $? -eq 0 ] ; then
	PUSH=`${SSHPASS_SCP} ${SCR_LOCAL_REL_NAME} ${DEFAULT_USER}@${IP}:${TARGET_DIR}`
	${SSHPASS_CMD} ${DEFAULT_USER}@${IP} chmod +x ${SCR_TARGET_NAME}
	OP=`${SSHPASS_CMD} ${DEFAULT_USER}@${IP} ${SCR_TARGET_NAME} ${@:3} `
	if [ $? -eq 0 ] ; then 
		echo "${IP},${OP}"
	else
		echo "$IP,SSH-FAIL"
	fi

else
	echo "$IP,NO-PING"
fi
