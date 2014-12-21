#!/bin/bash
IP=$1
BTC_ADDRESS=$2

# first, lets check that miner is accessible
ping -c 1 -W 1 $IP > /dev/null 2>&1
if [ $? -eq 0 ]
then
	# lets check that BTC address appears in cgminer config
    OUTPUT=`sshpass -p root ssh -o StrictHostKeyChecking=no root@${IP} grep ${BTC_ADDRESS} /etc/cgminer.conf 2>/dev/null`
	if [ -n "${OUTPUT}" ]
    then
        # return result as pair, ip address and cgminer
        # config file with BTC address we were looking for
		echo $IP,${OUTPUT}
	fi
fi
