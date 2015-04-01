#!/bin/bash
. $(dirname $0)/constants

usage()
{
	echo "Usage: $(fname) <ip_addr_file> [outputfilename|nofile]"
}

if [ $# -gt 2 ] || [ $# -lt 1 ] 
then
	usage
	exit 1
fi

SRC_IP_FILE=$1
SCRIPT_FILE=$(dirname $0)/minerstat
SCRIPT_PARMS=${@:3}


mkdir -p ${SCANS_REPO}
if [ $? -ne 0 ] ; then
	printf "Folder ${SCANS_REPO} doesn't exist, and cannot be created\n" 1>&2
	exit 2
fi

OUT_FILE=$2

if [ "${OUT_FILE}" == "nofile" ] ; then
	OUT_FILE=/dev/null
fi

if [ "${OUT_FILE}" == "" ] ; then
	OUT_FILE=${SCANS_REPO}/minerstats-`date +%m%d%H%M`.csv
fi

echo "OUT_FILE ${OUT_FILE}"

if [ ! -e ${SCRIPT_FILE} ] ; then
	echo "Script file ${SCRIPT_FILE} not found."
	exit 3
fi

if [ ! -e ${SRC_IP_FILE} ] ; then
	echo "IP Addresses File ${IP_FILE} not found."
	exit 4
fi

IP_FILE=$(mktemp)
cat $SRC_IP_FILE | cut -d ' ' -f 1 | cut -d , -f 1 | sed -e 's/\([^#]*\)#.*/\1/g' > $IP_FILE
dos2unix $IP_FILE

CUNT=0

index=0
while read line ; do
	MINERS[$index]="${line}"
	index=$(($index+1))
done < ${IP_FILE}

OIFS="$IFS"
IFS=' '
read -a PARAMS <<< "${SCRIPT_PARMS}"
IFS="$OIFS"


for MINER in ${MINERS[@]}; do
	CUNT=$((${CUNT} + 1))
	${SCRIPT_FILE} ${MINER} ${CUNT}	| tee -a ${OUT_FILE}
done

echo "=============================================================="
echo "  Scan Completed                                              "
echo ".                                                             ."
echo "  Results reside in file: ${OUT_FILE}  in a  csv(excel) format"
echo "=============================================================="


rm ${IP_FILE}

