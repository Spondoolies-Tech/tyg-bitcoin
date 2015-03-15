#!/bin/sh
MODEL=`cat /model_name`
HN=`hostname`
SN=${HN:6}
WALLET=`cat /etc/cgminer.conf | tr ',' '\n' | grep user | head -n 1 | cut -d : -f 2`
if [ `/usr/local/bin/spond-manager status` -eq 0 ] 
then 
	HASHRATE=0
	SUM_VOLT=0
	NUM_VOLT=0
	AVG_VOLT=0
	POWER=0
else
	POWER=`cat /tmp/asics | grep PSU | cut -d ' ' -f 2 | cut -d - -f 1 | /usr/bin/awk '{s+=$1} END {print s}'`
	HASHRATE=`cat /tmp/asics  | grep :HW: | cut -d : -f3 | cut -d G -f1`
	SUM_VOLT=`cat /tmp/production  | grep vlt | cut -d : -f4 | cut -d ' ' -f 1 | /usr/bin/awk '{s+=$1} END {print s}'`
	NUM_VOLT=`grep -c vlt: /tmp/production`
	AVG_VOLT=$((${SUM_VOLT}/${NUM_VOLT}))
fi

ENABLED_ASICS=$(awk 'BEGIN{count=0} {count+=gsub(/:0/,"")} END{print count}' /etc/mg_disabled_asics)

if [ ${ENABLED_ASICS} -ne 30 ] ; then
        DISABLED_ASICS_LIST=$(grep :1 /etc/mg_disabled_asics | tr '\n' ' ')
        REMOVED_ASICS_LIST=$(grep :2 /etc/mg_disabled_asics | tr '\n' ' ')
fi
CONF=$(cat /etc/mg_custom_mode)
echo "${SN},${MODEL},${WALLET},${HASHRATE},${NUM_VOLT},${AVG_VOLT},${POWER},EN_ASIC:${ENABLED_ASICS},DISABLED:${DISABLED_ASICS_LIST},${REMOVED_ASICS_LIST},${CONF}"

