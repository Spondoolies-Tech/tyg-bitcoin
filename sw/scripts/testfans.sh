#!/bin/sh

test_fan()  {
	#echo FAN- ${FAN} GPIO- ${GPIO}

	RC=0

	if [ "$(ls /sys/class/gpio/ |   grep -c gpio${GPIO})" == "0" ]; then
	    echo ${GPIO} > /sys/class/gpio/export
	fi
	echo "in" > /sys/class/gpio/gpio${GPIO}/direction

	count=0
	out=$(cat /sys/class/gpio/gpio${GPIO}/value)
	#echo $out
	fan_ok="Fail"
	while [ $count -le 100 ]
	do
		out_new=$(cat /sys/class/gpio/gpio${GPIO}/value)
		#echo $out_new
		if [ $out -ne $out_new ];
			then
			fan_ok="pass"
			break
		fi 
		count=$(( count+1 ))
	done

	if [ $fan_ok == "pass" ]; then
		echo -e "fan $FAN sense \e[32mPass \e[0m"
	else 
		echo -e "fan $FAN sense \e[41mFail! \e[0m"
		RC=$((1 << ${FAN}))
	fi

	return ${RC}
}

GPIO=68 ; FAN=1 ; test_fan ; err=$?

GPIO=67 ; FAN=2 ; test_fan ; err=$(($? + $err))

GPIO=66 ; FAN=3 ; test_fan ; err=$(($? + $err))

GPIO=69 ; FAN=4 ; test_fan ; err=$(($? + $err))

GPIO=60 ; FAN=5 ; test_fan ; err=$(($? + $err))

GPIO=30 ; FAN=6 ; test_fan ; err=$(($? + $err))

err=$((${err}>>1))
echo ERROR is ${err}
exit ${err}
