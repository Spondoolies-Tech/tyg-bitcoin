#!/bin/sh

# Upgrade software image.
#


. /etc/common-defs

__prog_name=`basename $0`

export PATH=$PATH:/usr/local/bin

SHAR=spon.shar


#success=0
einval=200
no_bootsource=201
not_upgradable=202
no_board_id=203
mount_fail=204
no_uimage=205
untar_fail=206
bad_signature=207
shar_missing=208


bootfrom=
board_id=`cat /board_ver`
software_version=`cat /fw_ver`


usage()
{
	cat<<-EOF
	Upgrade software image.
	Written by Vladik Goytin

	Usage: ${__prog_name} --help | --file=<FILE>

	 	--help		-- show this help
	 	--file		-- file name to install
	EOF
}



parse_args()
{
	opts="help,file:"
	temp=`getopt -o h --long ${opts} -n sign-digest.sh -- $@`

	[ $? -ne 0 ] && usage

	eval set -- "$temp"

	while :
	do
		case $1 in
		-h|--help)              usage; exit 0; shift ;;
		--file)			tar=$2;
					shift 2 ;;
		--)                     shift; break 2 ;;  # exit loop
		* )                     echo "unknown parameter $1"; return 1 ;;
	esac
	done
	[ x${tar} != x ]
}


detect_boot_source()
{
	# bootfrom=<BOOTSOURCE> is always the last element of kernel
	# command line so may get it this way.
	grep -q bootfrom /proc/cmdline &&
		bootfrom=`sed 's/.*bootfrom=//' /proc/cmdline` ||
			{ echo 'Cannot detect boot source.'; exit ${no_bootsource}; }
}

# Mount boot partition if not mounted already.
mount_boot_partition()
{
	grep -q /mnt/${bootfrom}-boot /proc/mounts ||
		mount /mnt/${bootfrom}-boot 2>/dev/null ||
			{ echo "Cannot mount /mnt/${bootfrom}-boot"; exit ${mount_fail}; }
}

untar()
{
	tar xf ${tar} 2>/dev/null ||
		{ echo "${tar}: Cannot untar."; exit ${untar_fail}; }
	( [ -f ${SHAR} ] && [ -f ${SHAR}.sign ] ) ||
		{ echo "Either ${SHAR} or ${SHAR}.sign is missing."; exit ${shar_missing}; }

	# No need in .TAR file anymore.
	rm -f ${tar}
}

verify_shar()
{
	verify-digest.sh --file=${SHAR} --public=${SPON_PUBLIC_KEY} ||
		{ echo "${SHAR}: bad signature"; exit ${bad_signature}; }
}


run_shar()
{
	export CURRENT_VERSION="${software_version}"
	export MEDIA=${bootfrom}
	# This is VERY dangerous. But anyway we do it.
	sh ${SHAR}
}

is_upgradeable()
{
	# Cannot upgrade software if it was taken from network.
	[ ${bootfrom} = 'sd' ] || [ ${bootfrom} = 'mmc' ]
}


main()
{
	parse_args $@ || { usage; exit ${einval}; }
echo "upgrading from " $tar
	detect_boot_source

	is_upgradeable
	if [ $? -eq 0 ]
	then
		mount_boot_partition
		echo "untarring "
		untar
		verify_shar
		run_shar
	else
		echo 'Software booted from network. Cannot upgrade.'
		exit ${not_upgradable}
	fi
}

main $@

