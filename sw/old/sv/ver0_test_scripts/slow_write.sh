#!/usr/bin/php


<?php
$usb = fopen ("/dev/ttyUSB0", "w+");
fwrite($usb, "TARGET_SET 00000000000001DE940000000000000000000000000000000000000000000000 END");
for($i = 0; $i < 100; $i++) {
	fwrite($usb, "DO_WORK D:2194261a T:9395e64d M:bed17115 M:228ea4732a3c9ba860c009cda7252b9161a5e75ec8c582a5f106abb3af41f790 END");
	usleep(10000);
}




/*
#echo "TARGET_SET 00000000000001DE940000000000000000000000000000000000000000000000 END" >>  /dev/ttyUSB0
while(!feof($usb))
  {
  	echo fgets($usb);
  }
fclose($usb)

*/

fclose ($usb);




