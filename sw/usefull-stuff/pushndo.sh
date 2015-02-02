#!/bin/bash
. $(dirname $0)/constants

SCR=$2
IP=$1
ping -c 1 -W 1 $IP > /dev/null 2>&1
if [ $? -eq 0 ] ; then
	PUSH=`${SSHPASS_SCP} $SCR ${DEFAULT_USER}@${IP}:/usr/local/bin `
	${SSHPASS_CMD} ${DEFAULT_USER}@${IP} chmod +x /usr/local/bin/$SCR 
	OP=`${SSHPASS_CMD} ${DEFAULT_USER}@${IP} /usr/local/bin/${SCR} ${@:3} `
	if [ $? -eq 0 ] ; then 
		echo ${IP},${OP}
	else
		echo $IP,SSH-FAIL
	fi

else
	echo $IP,NO-PING
fi
