#!/bin/sh

usage ()
{
	fn=$0
	fn="${fn##*/}"
	echo "Usage: $fn [-q|--quiet] [-a|--all] [-p|--pnr] [-m|--model] [-r|-revision] [-s|--serial]"
	exit 1
}

	OUT_Q=0
	OUT_A=0
	OUT_P=0
	OUT_M=0
	OUT_S=0
	OUT_R=0

if [ $# -eq 0  ] ; then
	OUT_P=1
	OUT_M=1
	OUT_S=1
	OUT_R=1
fi

while [ $# -gt 0 ] ;
do
	key="$1"
#	echo "key $key"
	shift
	case $key in
	    -h|--help)
	    usage
	    ;;
	    -a|--all)
	    OUT_A=1
	    ;;
	    -q|--quiet)
	    OUT_Q=1
	    ;;
	    -p|--pnr)
	    OUT_P=1
	    ;;
	    -m|--model)
	    OUT_M=1
	    ;;
	    -s|--serial)
	    OUT_S=1
	    ;;
	    -r|--revision)
	    OUT_R=1
	    ;;
	    *)
		    # unknown option
	    ;;
	esac
done

#if [ $OUT_A -eq 1  ] ; then
#	OUT_P=1
#	OUT_M=1
#	OUT_S=1
#	OUT_R=1
#fi

# open I2C bridge towards AC2DC controller
./i2cset -y 0 0x70 0x01

#verify AC2DC is in place
if [ ! 1 -eq `./i2cdetect -y -r 0 | grep -ci 5F` ] ; then
	echo "FAILED to detect AC2DC in I2C Bus!"
	echo "ERROR AC2DC DEVICE_NOT_EXIST"
	exit $((0x5f))
fi	

if [ ! 1 -eq `./i2cdetect -y -r 0 | grep -ci 57` ] ; then
	echo "FAILED to detect AC2DC EEPROOM in I2C Bus!"
	echo "ERROR AC2DC DEVICE_NOT_EXIST"
	exit $((0x57))	
fi	

PNR=`./read-acdc-eep.sh 52 66`
MODEL=`./read-acdc-eep.sh 87 90`
SERIAL=`./read-acdc-eep.sh 91 100`
REVISION=`./read-acdc-eep.sh 97 98`
ALL="${PNR}${MODEL}${SERIAL}"   
./i2cset -y 0 0x70 0x00

if [ $OUT_Q -eq 0 ] ; then
	H_PNR="AC2DC PNR: "
	H_MOD="AC2DC MOD: "
	H_SER="AC2DC SER: "
	H_REV="AC2DC REV: "
	H_ALL="AC2DC VPD: "
fi

if [ $OUT_P -eq 1 ] ; then echo "${H_PNR}${PNR}" ; fi
if [ $OUT_M -eq 1 ] ; then echo "${H_MOD}${MODEL}" ; fi
if [ $OUT_R -eq 1 ] ; then echo "${H_REV}${REVISION}" ; fi
if [ $OUT_S -eq 1 ] ; then echo "${H_SER}${SERIAL}" ; fi
if [ $OUT_A -eq 1 ] ; then echo "${H_ALL}${ALL}" ; fi

