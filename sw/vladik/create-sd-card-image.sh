#!/bin/sh

# Create SD Card image.
# Written by Vladik Goytin

SDCARD_SIZE=1
SDCARD=overo-sdcard.img
SECTOR_SIZE=512

create_image()
{
	dd bs=1M count=${SDCARD_SIZE} if=/dev/zero of=${SDCARD}
}


create_partition_table()
{
	losetup /dev/loop0 ${SDCARD}

	{
		echo 'n'	# Create a new partition ...
		echo 'p'	# ... of type 'primary'.
		echo '1'	# Partition number is 1.
		echo		# Default params, eat all space.
		echo		# Default params
		echo 't'	# Partition type number ...
		echo 'b'	# ... is 0xb (W95 FAT32)
		echo 'a'	# Make bootable ...
		echo '1'	# ... partition number 1.
		echo 'w'	# Write changes to media.
	} | fdisk /dev/loop0
}


format_fs()
{
	# Find when 1st partition start in terms of sectors.
	part1_start=`fdisk -lu /dev/loop0 | grep FAT32 | { read d d n d; echo $n; }`
	losetup -d /dev/loop0
	losetup -o $((part1_start * SECTOR_SIZE)) /dev/loop0 ${SDCARD}
	mkdosfs /dev/loop0
}


copy_files()
{
	mount /dev/loop0 /mnt
	cp overo/MLO.overo /mnt/MLO
	cp overo/u-boot.img.overo /mnt/u-boot.img
	umount -d /mnt
}

main()
{
	create_image
	create_partition_table
	format_fs
	copy_files
}

main $@
