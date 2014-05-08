#!/bin/sh

echoerr() { echo "$@" 1>&2; }

DURATION=10

if [ $# -gt 0 ] ; then
	if [ $1 -gt ${DURATION} ] ; then
		echo $1 bigger than ${DURATION} - lets switch
		DURATION=$1
	else
		echo $1 not bigger than ${DURATION} - lets NOT switch
	fi
fi


MGATE_CMD="miner_gate_arm"

MGATE_TEST_CMD="miner_gate_test_arm 39 100 100" 

MG_MOD=/etc/mg_work_mode

MODE_SILENT=0
MODE_NORMAL=1
MODE_TURBO=2

#if [ -e ${MG_MOD} ] ; then
#	mv ${MG_MOD} ${MG_MOD}.bu
#fi

#preps not expecting to have spond running, but just in case
spond-manager stop > /dev/null 2>&1

#echo ${MODE_SILENT} > ${MG_MOD}
echo ${MODE_TURBO} > ${MG_MOD}

#this is much more probable,
# we need to delete this, so we don't process garbage
rm -f /tmp/asics

# start miner_gate_arm
${MGATE_CMD} > /dev/null 2>&1  &
MGATE_ARM_PID=$!
echo "miner_gate_arm proc id is ${MGATE_ARM_PID}" 

#let it start
sleep 3

#then start miner_gate_test_arm
${MGATE_TEST_CMD} > /dev/null 2>&1  &
MGATE_TEST_PID=$!
echo "miner_gate_arm_test proc id is ${MGATE_TEST_PID}"

#now let it run some
sleep ${DURATION}

kill -9 ${MGATE_TEST_PID}
kill -9 ${MGATE_ARM_PID}

#rm ${MG_MOD}
#if [ -e ${MG_MOD}.bu ] ; then
#	mv ${MG_MOD}.bu ${MG_MOD}
#fi

echo ${MODE_TURBO} > ${MG_MOD}

if [ ! -e /tmp/asics ] ; then
	echoerr "Failed to generte ASICS table"
	exit 127
fi

cat /tmp/asics
 

 
