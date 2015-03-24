#!/bin/bash
. $(dirname $0)/constants

logger "FROMN CRON - PATH is $PATH"

TIMES=$1
DELAY=$2
PREF=$3
IPFILE=/tmp/0322-dc
OLDCD=$PWD

if [ $# -lt 2 ] ; then
	echo not enogh params
	exit 1
fi


if [ "$TIMES" -eq "$TIMES" ] 2>/dev/null ; then
	echo > /dev/null
else
	echo "Times $TIMES invalied"
	exit 3
fi

if [ "$DELAY" -eq "$DELAY" ] 2>/dev/null ;  then
	echo > /dev/null
else
	echo "Times $DELAY invalied"
	exit 4
fi


if [ $TIMES -lt 1 ] || [ $TIMES -gt 100 ] || [ $DELAY -lt 60 ] || [ $DELAY -gt 3600 ] ; then
	echo "Times $TIMES , Delay $DELAY - bad params"
	exit 2
fi

pushd ${BIN_REPO}

for (( i=1; i<=$TIMES ; i++ ))
do
	echo " inpar -show pushndo.sh ${IPFILE} miner_deep_conf.sh | grep ^1 | tee /home/remo/scans/${PREF}_$(date +%m%d_%H%M%S)_${i}_of_${TIMES}.csv "
	inpar -show pushndo.sh ${IPFILE} miner_deep_conf.sh | grep ^1 | tee /home/remo/scans/${PREF}_$(date +%m%d_%H%M%S)_${i}_of_${TIMES}.csv 
 	if [ $i -lt ${TIMES} ] ; then
		sleep ${DELAY}
	fi
done
 

popd
