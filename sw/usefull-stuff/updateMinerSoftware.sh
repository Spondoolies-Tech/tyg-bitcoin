#!/bin/bash
MINER_IP=$1
NEW_SW_PACKAGE_PATH=$2
CGMINER_TEMPLATE=$3

PACKAGE=$(basename ${NEW_SW_PACKAGE_PATH})
WD=$(dirname $0)
UNSUPPORTED_MODELS=SP10
USER=root
PASS=root

copy_package_to_miner() {
    sshpass -p ${PASS} scp -o StrictHostKeyChecking=no ${NEW_SW_PACKAGE_PATH} \
        ${USER}@${MINER_IP}:/tmp/${PACKAGE} >/dev/null 2>&1
    if [ ! $? == "0" ]
    then
        echo ${MINER_IP},"$0 failed"
        exit 1
    fi
}

copy_cgminer_template_to_miner() {
    sshpass -p ${PASS} scp -o StrictHostKeyChecking=no ${CGMINER_TEMPLATE} \
        ${USER}@${MINER_IP}:/etc/cgminer.conf.template >/dev/null 2>&1
    if [ ! $? == "0" ]
    then
        echo ${MINER_IP},"$0 failed"
        exit 1
    fi
}

installPackage() {
    sshpass -p ${PASS} ssh -o StrictHostKeyChecking=no ${USER}@${MINER_IP} \
        /usr/local/bin/upgrade-software-from-file.sh --file=/tmp/${PACKAGE} >/dev/null 2>&1
    if [ ! $? == "0" ]
    then
        echo ${MINER_IP},"$0 failed"
        exit 1
    fi
}

reboot_miner() {
    sshpass -p ${PASS} ssh -o StrictHostKeyChecking=no ${USER}@${MINER_IP} \
        /sbin/reboot >/dev/null 2>&1
    if [ ! $? == "0" ]
    then
        echo ${MINER_IP},"$0 failed"
        exit 1
    fi
}

# check that we have enough arguments to run our script
if [ $# -lt 2 ]
then
	echo "Wrong number of arguments"
	echo "Usage: $0 <miner ip> <spon_version.tar> < optional cgminer.template>"
	exit 1
fi

# check that miner is accessible
ping -c 1 -W 1 ${MINER_IP} >/dev/null 2>&1
if [ ! $? -eq 0 ]
then
    echo ${MINER_IP},"cannot access ${temp}"
    exit 1
fi

# check that /model_name file exists
MODEL=$(sshpass -p ${PASS} ssh -o StrictHostKeyChecking=no ${USER}@${MINER_IP} cat /model_name 2>/dev/null)
if [ -z "${MODEL}" ]
then
    MODEL=$(sshpass -p ${PASS} ssh -o StrictHostKeyChecking=no ${USER}@${MINER_IP} cat /model_id 2>/dev/null)
    echo ${MINER_IP},${MODEL},"UNSUPPORTED"
    exit 1
fi

# check if this model is supported
grep ${MODEL} -i <<< ${UNSUPPORTED_MODELS} >/dev/null 2>&1
if [ $? == "0" ]
then
    echo ${MINER_IP},${MODEL},"UNSUPPORTED_MODEL"
    exit 1
fi

# check if we can update cgminer template
if [ -n  "${CGMINER_TEMPLATE}" ]
then
    copy_cgminer_template_to_miner
fi

copy_package_to_miner
installPackage
reboot_miner

echo ${MINER_IP},${MODEL},"UPDATED"
