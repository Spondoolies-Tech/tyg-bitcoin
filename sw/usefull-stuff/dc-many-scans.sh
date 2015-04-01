#!/bin/bash
. $(dirname $0)/constants

TIMES=$2
DELAY=$3
PREF="$4$(date +%d)"
IPFILE=$1 


if [ $# -lt 3 ] ; then
	echo not enogh params
	echo "Usage: $(fname $0) <ipfile-abs-path> <times> <delay-seconds> {prefix_txt} "
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

if [ ! -e ${IPFILE} ] ; then
	echo " File ${IPFILE} not found. use absolute path"
	exit 5
fi

for (( i=1; i<=$TIMES ; i++ ))
do
	echo " inpar -show pushndo.sh ${IPFILE} miner_deep_conf.sh | grep ^1 | tee ${SCANS_REPO}/${PREF}_$(date +%d%H%M)_${i}_of_${TIMES}.csv "
	inpar -show pushndo.sh ${IPFILE} miner_deep_conf.sh | grep ^1 | tee ${SCANS_REPO}/${PREF}_$(date +%d%H%M)_${i}_of_${TIMES}.csv 
 	if [ $i -lt ${TIMES} ] ; then
		sleep ${DELAY}
	fi
done
 

popd
