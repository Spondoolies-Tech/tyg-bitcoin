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



spi_stuff()
{
	cd lib/firmware
	cp ${CUR_DIR}/../add-ons/BB-SPIDEV0-00A0.dtbo .
	ln -s -f BB-SPIDEV0-00A0.dtbo BB-SPI0-00A0.dtbo
	cd - 2>/dev/null
	cp -a ${CUR_DIR}/../add-ons/S60spi etc/init.d
	cp -a ${CUR_DIR}/../add-ons/S30hostname etc/init.d
	cp -a ${CUR_DIR}/../add-ons/.bashrc root
}

copy_all_spond_files() {
	cp ${CUR_DIR}/../add-ons/S90spondoolies etc/init.d
	cp ${CUR_DIR}/../add-ons/S87squid etc/init.d	
	cp ${CUR_DIR}/../add-ons/S85mount etc/init.d	
	cd ${TARGET_DIR}/usr/local/bin/
	ln -s -f ../../../etc/init.d/S90spondoolies spond  
	cd -
	#FPGA
	cp ${CUR_DIR}/../jtag/jam/jam usr/local/bin
	cp ${CUR_DIR}/../jtag/fpga-load.sh usr/local/bin
	if [ ! -d "spond-data" ]; then
		mkdir spond-data
	fi
	if [ ! -d "pids" ]; then
		mkdir pids
	fi
	cp ${CUR_DIR}/../add-ons/squid_top.jam spond-data
	
	#binaries
	cp ${CUR_DIR}/../../scripts/eeprom-read-hostname.sh usr/local/bin
	cp ${CUR_DIR}/../../scripts/read-mng-eeprom-stripped.sh usr/local/bin
	cp ${CUR_DIR}/../../spilib/miner_gate_arm usr/local/bin
	cp ${CUR_DIR}/../../spilib/miner_gate_test_arm usr/local/bin
	cp ${CUR_DIR}/../../cgminer-1/cgminer usr/local/bin
	cp ${CUR_DIR}/../../spilib/zabbix_reader/zabbix_reader_arm  usr/local/bin
	cp ${CUR_DIR}/../../spilib/hammer_reg/reg usr/local/bin
	#cp ${CUR_DIR}/../add-ons/mining_controller usr/local/bin
	cp ${CUR_DIR}/../add-ons/eeprom-provisioning.sh usr/local/bin
	
}


watchdog()
{
	cp -a ${CUR_DIR}/../add-ons/S15watchdog etc/init.d
}

memtester()
{
	cp -a ${CUR_DIR}/../memtester-4.3.0/memtester usr/local/bin
}

main()
{
	set -e
	cd ${TARGET_DIR}

	add_dirs
	add_dropbear_keys
	cleanup
	fix_mdev_conf
	copy_all_spond_files
	spi_stuff
	watchdog
	memtester
}

main $@
