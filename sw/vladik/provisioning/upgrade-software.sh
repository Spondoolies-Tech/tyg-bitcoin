#!/bin/sh

# Upgrade software image.
#
# If URL https://mydomain.com/thefile should be downloaded
# it takes also https://mydomain.com/thefile.sign
#
# Written by Vladik Goytin


. /etc/common-defs

__prog_name=`basename $0`



# Return code (errors). Start frm 200 to leave curl error code intact.
#success=0
einval=200
no_bootsource=201
not_upgradable=202
no_board_id=203
mount_fail=204
no_uimage=205
download_fail=206
bad_signature=207


url=
shar=
bootfrom=
board_id=
software_version=


usage()
{
	cat<<-EOF
	Upgrade software image.
	Written by Vladik Goytin

	Usage: ${__prog_name} --help | --url=<URL>

	 	--help		-- show this help
	 	--url		-- url's URL to download

	Examples:
	 	${__prog_name} --url=https://mydomain.com/thefile
	 		-- download file from URL https://mydomain.com/thefile
	 		   along with its signed digest
	 		   https://mydomain.com/thefile.sign
	EOF
}



parse_args()
{
	opts="help,url:"

	temp=`getopt -o h --long ${opts} -n sign-digest.sh -- $@`
	[ $? -ne 0 ] && usage
	eval set -- "$temp"

	while :
	do
		case $1 in
		-h|--help)              usage; exit 0; shift ;;
		--url)			url=$2;
					shar=`basename ${url}`
					shift 2 ; break;;
		--)                     shift; break 2 ;;  # exit loop
		* )                     echo "unknown parameter $1"; return 1 ;;
	esac
	done

	[ x${url} != x ]
}


detect_boot_source()
{
	# bootfrom=<BOOTSOURCE> is always the last element of kernel
	# command line so may get it this way.

	# on 10.0.0.20, 13/03/2014, this returns: console=ttyO0,115200n8 ip=::::::none::
	# so we cannot get bootsource with this command
	#grep -q bootfrom /proc/cmdline &&
	#	bootfrom=`sed 's/.*bootfrom=//' /proc/cmdline` ||

	#instead, lets look for uImage under /mnt. 
	bootfrom=`find /mnt -name uImage | head -1 | sed 's/-.*//;s/^.*\///'` ||
			{ echo 'Cannot detect boot source.'; exit ${no_bootsource}; }
}


get_board_id()
{
	# Get board ID from EEPROM. Starts at offset 84, 12 bytes long.
	board_id=`dd bs=12 skip=7 count=1 if=${EEPROM_DEVICE} 2>/dev/null` ||
		{ echo "Cannot read Board ID."; exit ${no_board_id}; }
}


# Mount boot partition if not mounted already.
mount_boot_partition()
{
	grep -q /mnt/${bootfrom}-boot /proc/mounts ||
		mount /mnt/${bootfrom}-boot 2>/dev/null ||
			{ echo "Cannot mount /mnt/${bootfrom}-boot"; exit ${mount_fail}; }
}

get_software_version()
{
	[ -f /mnt/${bootfrom}-boot/uImage ] &&
		software_version=`mkimage -l /mnt/${bootfrom}-boot/uImage | \
			grep 'Image Name' | { read d d n; echo $n; }` ||
		{ echo "Cannot find /mnt/${bootfrom}-boot/uImage."; exit ${no_uimage}; }
}


download_shar()
{
	cd /tmp
	download-file.sh --url=${url}						\
			--query="id=${board_id}&ver=${software_version}" ||	\
		{ rc=$?; echo "${url} download fail, rc = ${rc}"; exit ${rc}; }
}


verify_shar()
{
	verify-digest.sh --file=${shar} --public=${SPON_PUBLIC_KEY} ||
		{ echo "${shar}: bad signature"; exit ${bad_signature}; }
}


run_shar()
{
	export CURRENT_VERSION="${software_version}"
	export MEDIA=${bootfrom}
	# This is VERY dangerous. But anyway we do it.
	sh ${shar}
}

is_upgradeable()
{
	# Cannot upgrade software if it was taken from network.
	[ ${bootfrom} = 'sd' ] || [ ${bootfrom} = 'mmc' ]
}


main()
{
	parse_args $@ || { usage; exit ${einval}; }

	detect_boot_source

	is_upgradeable
	if [ $? -eq 0 ]
	then
		get_board_id
		mount_boot_partition
		get_software_version
		download_shar
		verify_shar
		run_shar
	else
		echo 'Software booted from network. Cannot upgrade.'
		exit ${not_upgradable}
	fi
}

main $@