#!/bin/sh

# This file exists mostly for USB wireless NICs.

# Kernel supplies certain environment variables:
#
#	- INTERFACE
#	- ACTION
#	- DEVTYPE

add_wifi()
{
	sleep 2
	ifup ${INTERFACE}
}

remove_wifi()
{
	sleep 2
	ifdown ${INTERFACE}

	# To be on the safe side.
	pkill wpa_supplicant
	pkill -f "udhcpc.*${INTERFACE}"
}


main()
{
	( [ x${INTERFACE} = x ] || [ x${DEVTYPE} != xwlan ] ) && return

	# Continue only if the interface is auto-configured.
	grep -q "^auto.*${INTERFACE}" /etc/network/interfaces || return

	case ${ACTION} in
	add)
		add_wifi
		;;
	remove)
		remove_wifi
		;;
	*)
		echo "Unknown action: ${ACTION}"
		exit 1
	esac
}

main $@
