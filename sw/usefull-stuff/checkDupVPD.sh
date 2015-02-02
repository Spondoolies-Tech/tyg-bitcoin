#!/bin/bash
. $(dirname $0)/constants



MINER_IP=$1

READ_MNG_VPD=/usr/local/bin/readmngvpd.sh
READ_BOX_VPD=/usr/local/bin/readboxvpd.sh
GET_MAC=/usr/local/bin/getmac.sh

XPATH="export PATH=/bin:/sbin:/usr/bin:/usr/sbin:/usr/bin/X11:/usr/local/bin"

MNG_VPD=`sshpass -p ${PASS} ssh -o StrictHostKeyChecking=no ${USER}@${MINER_IP} "${XPATH} ; ${READ_MNG_VPD}"`
BOX_VPD=`sshpass -p ${PASS} ssh -o StrictHostKeyChecking=no ${USER}@${MINER_IP} "${XPATH} ; ${READ_BOX_VPD}"`
MAC=`sshpass -p ${PASS} ssh -o StrictHostKeyChecking=no ${USER}@${MINER_IP} "${XPATH} ; ${GET_MAC}"`

if [ "${MNG_VPD}"=="${BOX_VPD}" ] ; then
	echo ${MINER_IP},${MAC},GOOD,${MNG_VPD},${BOX_VPD}
else
	echo ${MINER_IP},${MAC},BAD,${MNG_VPD},${BOX_VPD}
fi


