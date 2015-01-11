#!/bin/bash

SCR=$2
IP=$1
ping -c 1 -W 1 $IP > /dev/null 2>&1
if [ $? -eq 0 ] ; then
	PUSH=`sshpass -p root scp -o StrictHostKeyChecking=no $SCR root@${IP}:/usr/local/bin `
	sshpass -p root ssh -o StrictHostKeyChecking=no root@${IP} chmod +x /usr/local/bin/$SCR 
	OP=`sshpass -p root ssh -o StrictHostKeyChecking=no root@${IP} /usr/local/bin/${SCR} ${@:3} `
	if [ $? -eq 0 ] ; then 
		echo ${IP},${OP}
	else
		echo $IP,SSH-FAIL
	fi

else
	echo $IP,NO-PING
fi
