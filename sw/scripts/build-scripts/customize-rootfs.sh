#!/bin/sh

# Rootfs customization.


# This is provided by Buildroot
TARGET_DIR=$1
CUR_DIR=${PWD}

TFTP_SERVER_IP=1.1.1.1
MY_IP=1.1.1.2

DIRS_TO_ADD='usr/local/bin lib/modules lib/firmware'
TO_REMOVE='etc/init.d/S??urandom usr/lib/pkgconfig'


add_dirs()
{
	mkdir -p ${DIRS_TO_ADD}
}

cleanup()
{
	rm -rf ${TO_REMOVE}
}

add_dropbear_keys()
{
	mkdir -p etc/dropbear
	cp ${CUR_DIR}/../add-ons/dropbear_*_host_key etc/dropbear
	chmod 600 etc/dropbear/*
}

fix_mdev_conf()
{
	sed -i '/=.*\/$/d' etc/mdev.conf
}

zabbix_agent()
{
	cp ${CUR_DIR}/../zabbix-2.0.8/src/zabbix_agent/zabbix_agentd usr/local/bin
}

cgminer()
{
	cp ${CUR_DIR}/../cgminer-3.4.3/cgminer usr/local/bin
}


spi_stuff()
{
	cd lib/firmware
	cp ${CUR_DIR}/../add-ons/BB-SPIDEV0-00A0.dtbo .
	ln -s -f BB-SPIDEV0-00A0.dtbo BB-SPI0-00A0.dtbo

	cd - 2>/dev/null
	cp -a ${CUR_DIR}/../add-ons/S60spi etc/init.d
}

fpga_stuff()
{
	cp ${CUR_DIR}/../jtag/jam/jam usr/local/bin
	cp ${CUR_DIR}/../jtag/fpga-load.sh usr/local/bin
}

main()
{
	set -e
	cd ${TARGET_DIR}

	add_dirs
	add_dropbear_keys
	cleanup
	fix_mdev_conf
	zabbix_agent
	cgminer
	spi_stuff
	fpga_stuff
}

main $@
