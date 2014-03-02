#!/bin/sh
ifconfig eth0 |  grep eth0 | sed -r 's/(\s+)/,/g' | cut -d , -f 5

