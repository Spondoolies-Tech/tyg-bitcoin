#!/bin/sh

start=$(($1))
end=$(($2))
i=$start
#echo "i $i start $start end $end"

while [ $end -ge $i ] ; do
  val=`./i2cget -y 0 0x57 $i`
  cc=$(printf "\\$(printf '%03o' ${val})")
  text="${text}${cc}"
  let i=i+1
done

echo "${text}"
