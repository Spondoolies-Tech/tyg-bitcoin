#!/bin/bash

#################################################################
#
# Remo ( remo@spondoolies-tech.com )
# 28-Jan 2015
#
# Perform group update of miners
# push tar package (taken from firmware server) into on one of the mimners
# and send update commands to all the miners, to upgrade from that miner
# this is very efficient when controller and miners are not on the same LAN
#################################################################
. $(dirname $0)/constants
if [ ! "$1" == "" ] && [ -f $1 ] ; then
	IPF=$1
	shift
else
	echo "No Valid IP table file given $1 !!"
	exit 4
fi
THREADS=10
if [ ${THREADS} -gt ${MAX_PROC} ] ; then
	THREADS=${MAX_PROC}
fi

VER_NUM=$1

i=0 
SELECTED_IP="NONE"

for ST in $(cat $IPF | tr ' ' ',' | tr '"' ',' | cut -d , -f 1 |  sed -e 's/\([^#]*\)#.*/\1/g') 
do 
	MINERS_IPS[$i]=${ST}	
	i=$(($i+1)) 
	if [ "$SELECTED_IP" == "NONE" ] ; then
		ping -c  1 -W 1 $ST > /dev/null 2>&1
		if [ $? -eq 0 ] ; then
			SELECTED_IP=$ST
		fi
	fi
done


echo "total of ${#MINERS_IPS[@]} - ${MINERS_IPS[0]} ... ${MINERS_IPS[$((${#MINERS_IPS[@]}-1))]} "
#echo "total of ${#MINERS_IPS[@]} - ${MINERS_IPS[@]} "



if [ "$SELECTED_IP" == "NONE" ] ; then
	echo "No Responsive IP found! Aborting"
	exit 1
else 
	echo "Selected IP is $SELECTED_IP" ;
 #       exit 0 	
fi

TAR_FILE=${IMG_REPO}/spon_${VER_NUM}.tar

if [ -f ${TAR_FILE} ] ; then
echo	 whooo hooo
else
	echo "${TAR_FILE} not exist boo hoo - lets get it!!"
	wget http://storage.googleapis.com/spond_firmware/spon_${VER_NUM}.tar -O ${TAR_FILE}
	DLSUC=$?
	if [ $DLSUC -ne 0 ] ; then
		echo "We failed to download tar file of version ${VER_NUM} - Aborting"
		rm -f ${TAR_FILE}
		exit 3
	else
		echo " tar of version ${VER_NUM} downloaded to server. let's continue. "
		echo " and may schwartz be with US!!"
	fi
fi

echo "scp ${TAR_FILE} ${DEFAULT_USER}@${SELECTED_IP}:/var/www/img/spon_${VER_NUM}.tar"
${SSHPASS_SCP} ${TAR_FILE} ${DEFAULT_USER}@${SELECTED_IP}:/var/www/img/spon_${VER_NUM}.tar


echo "lets got over miners!!"
j=0
while [ $j -lt ${#MINERS_IPS[@]} ] ; do
#	echo " of mani J loop - j = ${j} "
        unset mypids
        for (( i = 0 ; i < ${THREADS} && j < ${#MINERS_IPS[@]} ; i++ )) do
		TARGET=${MINERS_IPS[$j]}
#		echo "$j - $TARGET"
		
#		echo "  --->>>>> ssh ${DEFAULT_USER}@${TARGET} /usr/local/bin/upgrade-software.sh --url=http://${SELECTED_IP}/img/spon_${VER_NUM}.tar" 
		${SSHPASS_CMD} ${DEFAULT_USER}@${TARGET} /usr/local/bin/upgrade-software.sh --url=http://admin:${WEB_PASS}@${SELECTED_IP}/img/spon_${VER_NUM}.tar &
                mypids[$i]=$!
                total_pids[$j]=$!
                j=$(($j+1))
        done

	echo "finished small blocks ${THREADS} lets wait for them"
	echo ${mypids[@]}
        wait $(echo ${mypids[@]})

done

TOTAL_MINERS=${#MINERS_IPS[@]} 
unset FUCK_LIST
unset SUCK_LIST
SUCKOUNT=0
FUCKOUNT=0

for (( i = 0 ; i < ${TOTAL_MINERS} ; i++ )) do
	wait ${total_pids[$i]}
	SUC=$?
	TRG=${MINERS_IPS[$i]}
	if [ ${SUC} -eq 0 ] ; then
		echo "${TRG},UPDATED"
        	SUCK_LIST[${SUCKOUNT}]=${TRG}
	        SUCKOUNT=$(($SUCKOUNT+1))
	else
		FUCK_LIST[${FUCKOUNT}]=${TRG}
                FUCKOUNT=$(($FUCKOUNT+1))
        	echo "${TRG},FAILED"
	fi
done


echo -e
echo -e
echo "Total Miners in List ${TOTAL_MINERS}"
echo "Succeeded ${SUCKOUNT} , Failed ${FUCKOUNT} "

if [ ${FUCKOUNT} -gt 0 ] ; then
	echo -e
	echo -e
	echo "Failed to upgrade these IP "

	for TARGET in ${FUCK_LIST[@]}
	do
		echo "  $TARGET"
	done	
fi

if [ ${SUCKOUNT} -gt 0 ] ; then
	echo -e
	echo -e
	for TARGET in ${SUCK_LIST[@]}
	do
		${SSHPASS_CMD} ${DEFAULT_USER}@${TARGET} /sbin/reboot &
		echo "$TARGET,UPDATED (Rebooting...)"
	done
fi
