#!/bin/bash

# Rootfs customization.

. ../add-ons/common-defs

# This is provided by Buildroot
TARGET_DIR=$1
CUR_DIR=${PWD}

TFTP_SERVER_IP=1.1.1.1
MY_IP=1.1.1.2

DIRS_TO_ADD='usr/local/bin usr/local/lib lib/modules lib/firmware'
DIRS_TO_ADD=${DIRS_TO_ADD}' etc/cron.d/crontabs'
DIRS_TO_ADD=${DIRS_TO_ADD}" ${MP_SD_BOOT} ${MP_MMC_BOOT} ${MP_MMC_CONF} ${MP_NFS}"
TO_REMOVE='etc/init.d/S??urandom usr/lib/pkgconfig'
TO_REMOVE=${TO_REMOVE}' sbin/fsck.xfs sbin/xfs_repair usr/sbin/xfs*'

tmpfile=`mktemp`

add_dirs()
{
	# Remove leading '/' in a string like "bla-bla /mnt/mmc"
	mkdir -p ${DIRS_TO_ADD// \// }
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
	if [ ! -d "etc/profile.d/" ]; then
		mkdir etc/profile.d/
	fi
	cp -a ${CUR_DIR}/../add-ons/bashrc.sh etc/profile.d/
}

copy_all_spond_files() {
	if [ ! -d "var/www" ]; then
		mkdir var/www
	fi
	
	cp ${CUR_DIR}/../../../../minepeon/http/* var/www -r

	#cp ${CUR_DIR}/../add-ons/S99startup etc/init.d
	cp ${CUR_DIR}/../add-ons/S90spondoolies etc/init.d
	cp ${CUR_DIR}/../add-ons/S87squid etc/init.d	
	cp ${CUR_DIR}/../add-ons/S42checkip etc/init.d/S42checkip
	cp ${CUR_DIR}/../add-ons/ui.pwd etc	
	cp ${CUR_DIR}/../add-ons/cgminer.conf etc
	#FPGA
	cp ${CUR_DIR}/../jtag/jam/jam usr/local/bin
	cp ${CUR_DIR}/../jtag/fpga-load.sh usr/local/bin
	if [ ! -d "spond-data" ]; then
		mkdir spond-data
	fi
	cp ${CUR_DIR}/../add-ons/squid_top.jam spond-data
	cp ${CUR_DIR}/../add-ons/spond-manager usr/local/bin
	cp ${CUR_DIR}/../arm-binaries/*  usr/local/bin
	
	#binaries
	cp ${CUR_DIR}/../../scripts/eeprom-read-hostname.sh usr/local/bin
	cp ${CUR_DIR}/../../scripts/rff usr/local/bin
	cp ${CUR_DIR}/../../scripts/wtf usr/local/bin
	cp ${CUR_DIR}/../../scripts/mainvpd usr/local/bin
	cp ${CUR_DIR}/../../scripts/mbtest usr/local/bin
	cp ${CUR_DIR}/../../scripts/getmac.sh usr/local/bin
	cp ${CUR_DIR}/../../scripts/ac2dcvpd usr/local/bin
	cp ${CUR_DIR}/../../scripts/readmngvpd.sh usr/local/bin
	cp ${CUR_DIR}/../../scripts/readboxvpd.sh usr/local/bin
	cp ${CUR_DIR}/../../scripts/writemngvpd.sh usr/local/bin
	cp ${CUR_DIR}/../../scripts/writeboxvpd.sh usr/local/bin
	#cp ${CUR_DIR}/../../scripts/read-mng-eeprom-stripped.sh usr/local/bin
	cp ${CUR_DIR}/../../spilib/miner_gate/miner_gate_arm usr/local/bin
	cp ${CUR_DIR}/../../spilib/miner_gate/mg_version ./
	cp ${CUR_DIR}/../add-ons/board_ver ./
	cp ${CUR_DIR}/../add-ons/fw_ver ./
	cp ${CUR_DIR}/../../../../cg-miner-git/cgminer/cgminer usr/local/bin
	cp ${CUR_DIR}/../../spilib/miner_gate_test_arm usr/local/bin
	cp ${CUR_DIR}/../../spilib/zabbix_reader/zabbix_reader_arm  usr/local/bin
	cp ${CUR_DIR}/../../spilib/hammer_reg/reg usr/local/bin
	#cp ${CUR_DIR}/../add-ons/mining_controller usr/local/bin
	rm etc/resolv.conf
	cp ${CUR_DIR}/../add-ons/resolv.conf etc/
	cp ${CUR_DIR}/../add-ons/eeprom-provisioning.sh usr/local/bin
	date > build_date.txt

	#php
	if [ ! -d "opt/minepeon" ]; then
		mkdir opt/minepeon
		mkdir opt/minepeon/etc
	fi
	cp ${CUR_DIR}/../add-ons/miner.conf opt/minepeon/etc/
	cp ${CUR_DIR}/../add-ons/minepeon.conf opt/minepeon/etc/
	cp ${CUR_DIR}/../add-ons/php.ini etc/
}

common_defs()
{
	cp -a ${CUR_DIR}/../add-ons/common-defs etc
}

watchdog()
{
	cp -a ${CUR_DIR}/../add-ons/S15watchdog etc/init.d
}

memtester()
{
	cp -a ${CUR_DIR}/../memtester-4.3.0/memtester usr/local/bin
}

emmc()
{
	cp -a ${CUR_DIR}/../add-ons/emmc-mbr usr/local/lib
	cp -a ${CUR_DIR}/../add-ons/prepare-emmc.sh usr/local/bin
}

web_server()
{
	cp -a ${CUR_DIR}/../../../../minepeon/http/* var/www/ -rf
	cp -a ${CUR_DIR}/../add-ons/lighttpd.conf etc/lighttpd
	cp -a ${CUR_DIR}/../add-ons/spondoolies-cert.pem root

	for m in trigger_b4_dl status ssi simple setenv secdownload scgi proxy	\
		mysql_vhost magnet flv_streaming extforward expire evhost	\
		evasive compress cml cgi alias accesslog userdir usertrack	\
		webdav access.so
	do
		rm -f usr/lib/lighttpd/mod_${m}.so
	done
}

generate_fstab()
{
	cat<<-EOF
	# SD Card exists mounts
	/dev/mmcblk0p1	${MP_SD_BOOT}	vfat	defaults,noauto,noatime	0 0 # SD=yes
	/dev/mmcblk1p1	${MP_MMC_BOOT}	vfat	defaults,noauto,noatime	0 0 # SD=yes
	/dev/mmcblk1p2	${MP_MMC_CONF}	xfs	defaults,noauto,noatime	0 0 # SD=yes
	#/dev/mmcblk1p3	/var/log	xfs	defaults,noauto,noatime	0 0 # SD=yes
	# SD Card does NOT exist mounts
	/dev/mmcblk0p1	${MP_MMC_BOOT}	vfat	defaults,noauto,noatime	0 0 # SD=no
	/dev/mmcblk0p2	${MP_MMC_CONF}	xfs	defaults,noauto,noatime	0 0 # SD=no
	#/dev/mmcblk0p3	/var/log	xfs	defaults,noauto,noatime	0 0 # SD=no

	unionfs		/etc		unionfs	noauto,dirs=${MP_MMC_CONF}/etc=rw:/etc=ro 0 0
	EOF
}

mounts()
{
	grep -v AUTOGENERATED etc/fstab > ${tmpfile}
	mv ${tmpfile} etc/fstab
	generate_fstab | sed 's/$/ # AUTOGENERATED/' >> etc/fstab
	cp -a ${CUR_DIR}/../add-ons/S12mount etc/init.d
	cp -a ${CUR_DIR}/../add-ons/S45nfs etc/init.d
	cp -a ${CUR_DIR}/../add-ons/default.script usr/share/udhcpc
	cp -a ${CUR_DIR}/../add-ons/interfaces etc/network

}

sw_upgrade()
{
	cp -f ${CUR_DIR}/../add-ons/spondoolies-pub.pem etc
	for f in download-file.sh upgrade-software.sh verify-digest.sh
	do
		cp -a ${CUR_DIR}/../provisioning/${f} usr/local/bin
	done
}


cron()
{
	cp -a ${CUR_DIR}/../add-ons/S47cron etc/init.d
	cp -a ${CUR_DIR}/../add-ons/S13misc etc/init.d
	cp ${CUR_DIR}/../../../../minepeon/etc/cron.d/5min/RECORDHashrate etc/cron.d
	cp ${CUR_DIR}/../../../../minepeon/etc/cron.d/hourly/pandp_register.sh etc/cron.d

	# Should be deleted as it is symlinked to /tmp/rrd in runtime.
	rm -rf var/www/rrd

	# Run every minute
	echo '0,5,10,15,20,25,30,35,40,45,50,55 * * * * /usr/bin/php /etc/cron.d/RECORDHashrate' > etc/cron.d/crontabs/root
	#echo '0 * * * * /etc/cron.d/pandp_register.sh' >> etc/cron.d/crontabs/root
}

# Ugly hack to add php-rrd to the image.
rrd()
{
	xzcat ${CUR_DIR}/../rrd/php-rrd.tar.xz | tar -xf -
}

ntp()
{
	cp -a ${CUR_DIR}/../add-ons/S43ntp etc/init.d
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
	common_defs
	spi_stuff
	watchdog
	memtester
	emmc
	web_server
	mounts
	sw_upgrade
	cron
	#ntp
	rrd
}

main $@
