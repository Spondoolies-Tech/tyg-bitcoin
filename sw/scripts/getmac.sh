#!/bin/sh
IFCONFOG=/sbin/ifconfig
GREP=/bin/grep
SED=/bin/sed
CUT=/usr/bin/cut
${IFCONFOG} eth0 |  ${GREP} eth0 | ${SED} -r 's/(\s+)/,/g' | ${CUT} -d , -f 5

