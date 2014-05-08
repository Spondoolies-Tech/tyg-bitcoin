#!/bin/sh

# Create wpa_supplicant configuration file.
# Passphrase supplied from stdin.


usage()
{
	cat<<-EOF
	Create wpa_supplicant configuration file.
	Usage:
	 	ESSID="<essid>" KEY_MGMT=<key mgmt> PROTO=<proto>\\
	 		PAIRWISE_CIPHERS="<pairwise ciphers>" GROUP_CIPHERS="<group ciphers>" $0
	EOF
}


wifi_conf_header()
{
	cat<<-EOF
	# WPA Supplicant configuration file
	# Generated on `date -u`
	# DO NOT MODIFY!
	#

	ctrl_interface=/var/run/wpa_supplicant

	network={
	 	ssid="${ESSID}"
	EOF

}

secured_wifi_conf()
{
	cat<<-EOF
	 	key_mgmt=${KEY_MGMT}
	 	proto=${PROTO}
	 	pairwise=${PAIRWISE_CIPHERS}
	 	group=${GROUP_CIPHERS}
	  	`wpa_passphrase "${ESSID}" | grep '^	psk=' | sed 's/^\t//'`
	}
	EOF
}


unsecured_wifi_conf()
{
	cat<<-EOF
	 	key_mgmt=NONE
	}
	EOF
}

test_environment()
{
	[ x"${ESSID}" != x ] && [ x"${KEY_MGMT}" != x ] &&
		( [ "${KEY_MGMT}" = 'NONE' ] ||
		  ( [ x"${PROTO}" != x ] && [ x"${PAIRWISE_CIPHERS}" != x ] &&
		    [ x"${GROUP_CIPHERS}" != x ] )
		)
}


main()
{
	test_environment || { usage; exit 1; }

	{
		wifi_conf_header
		[ "${KEY_MGMT}" = 'NONE' ] && unsecured_wifi_conf || secured_wifi_conf
	} > /etc/wifi.conf
}

main $@
