#!/bin/sh

# Prepare eMMC.
# Written by Vladik Goytin

set -e
set -x

. /etc/common-defs

SDCARD_DEVICE=/dev/mmcblk0
EMMC_DEVICE=/dev/mmcblk1
MBR=/usr/local/lib/emmc-mbr
MBR_SIZE=512
tmpfile=`mktemp`


# Certain services prevent unmounting: stop them.
stop_services()
{
	spond-manager stop
	sleep 1
	/etc/init.d/S47cron stop & 
	sleep 1
	pkill klogd &
	sleep 1
	pkill lighttpd &
	sleep 1
	pkill netplugd &
	sleep 1
	pkill syslogd &
	sleep 1
	pkill udhcpc &
	sleep 1
	pkill crond &
	sleep 1
}

unmount_unionfs()
{
	mount | grep 'unionfs' |
	{
		while read rest rest mountpoint rest
		do
			umount ${mountpoint}
		done
	}

}


# In a very unlikely situation where eMMC already mounted unmount
# its filesystems first.
unmount_emmc()
{
	mount | grep ${EMMC_DEVICE} |
	{
		while read dev rest
		do
			umount ${dev}
		done
	}

	mount | grep ${SDCARD_DEVICE} |
	{
		while read dev rest
		do
			umount ${dev}
		done
	}

}


place_mbr()
{
	# First erase existing MBR to be on the safe side ...
	dd bs=${MBR_SIZE} count=1 if=/dev/zero of=${EMMC_DEVICE} 2>/dev/null

	# ... now place our MBR.
	dd bs=${MBR_SIZE} count=1 if=${MBR} of=${EMMC_DEVICE} 2>/dev/null
}

# Force kernel to re-read eMMC partition table.
reread_partition_table()
{
	cd /sys/bus/mmc/drivers/mmcblk

	# eMMC is always seen as mmc1:XXXX. Find out the exact name
	# of the device.
	# # Remove "./"
	emmc_dev=`find -name 'mmc1:*' | sed 's/\.\///'`	

	# Rebind the device to the driver
	echo ${emmc_dev} > unbind
	echo ${emmc_dev} > bind

}

format_partitions()
{
	# eMMC has 3 partitions: VFAT and 2 XFS
	mkfs.vfat ${EMMC_DEVICE}p1
	mkfs.xfs -f -q ${EMMC_DEVICE}p2
	mkfs.xfs -f -q ${EMMC_DEVICE}p3
}

create_dirs()
{
	mount ${EMMC_DEVICE}p2 ${MP_MMC_CONF}
	mkdir ${MP_MMC_CONF}/etc #${MP_MMC_CONF}/rrd
	ln -s /tmp ${MP_MMC_CONF}/rrd
	umount ${MP_MMC_CONF}
	
}

copy_files()
{
	mount ${SDCARD_DEVICE}p1 ${MP_SD_BOOT}
	mount ${EMMC_DEVICE}p1 ${MP_MMC_BOOT}
	cp ${MP_SD_BOOT}/upgrade/MLO					\
		${MP_SD_BOOT}/upgrade/u-boot.img			\
		${MP_SD_BOOT}/upgrade/uImage				\
		${MP_SD_BOOT}/upgrade/spondoolies.dtb ${MP_MMC_BOOT}
	umount ${MP_MMC_BOOT}
	umount ${MP_SD_BOOT}
}

start_counting()
{
	echo -n 'Preparing eMMC. Please wait '
	while [ -f ${tmpfile} ]
	do
		echo -n '.'
		sleep 1
	done
}


stop_counting()
{
	rm -f ${tmpfile}
	echo ' done.'
}

check_for_sdcard()
{
	# Only if SD Card present this program is allowed to work.
	( [ -b ${SDCARD_DEVICE} ] && [ -b ${EMMC_DEVICE} ] ) ||
		{ echo 'SD Card is not present. Exiting.'; exit 1; }
}

main()
{
	check_for_sdcard
	#start_counting &
	stop_services
	unmount_unionfs
	unmount_emmc
	place_mbr
	reread_partition_table
	format_partitions
	create_dirs
	copy_files
	#stop_counting
}

main $@
