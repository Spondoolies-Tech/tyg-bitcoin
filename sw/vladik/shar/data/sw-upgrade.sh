#!/bin/sh
#
# Written by Vladik Goytin

# Mandatory environment provided by a caller:
#
#	- CURRENT_VERSION
#	- MEDIA


curdir=


get_new_version()
{
	#new_version=`mkimage -l uImage | \
	#	grep 'Image Name' | { read d d n; echo $n; }`
	new_version=`cat ${curdir}/fw_ver`

}


print_msg_start()
{
	echo -n "Upgrading ${CURRENT_VERSION} to ${new_version} ..."
}

print_msg_end()
{
	echo ' done.'
	echo "Please reboot the miner."
}

do_upgrade()
{
	# Boot partition already mounted. Go there.
	cd /mnt/${MEDIA}-boot
	
	mv -f u-boot.img u-boot.old ||
		{ echo 'Cannot move u-boot.img to u-boot.old'; exit 220; }

	mv -f uImage uImage.old ||
		{ echo 'Cannot move uImage to uImage.old'; exit 220; }

	mv -f spondoolies.dtb spondoolies.dtb.old ||
		{ echo 'Cannot move spondoolies.dtb to spondoolies.dtb.old';	\
			mv uImage.old uImage; exit 221; }
	
	mv -f ${curdir}/u-boot.img u-boot.img ||
		{ echo 'Cannot copy new u-boot.img, revert to the old version';	\
			mv uImage.old uImage;					\
			mv u-boot.old u-boot.img;				\
			mv spondoolies.dtb.old spondoolies.dtb;			\
			exit 222; }


	mv -f ${curdir}/uImage uImage ||
		{ echo 'Cannot copy new uImage, revert to the old version';	\
			mv uImage.old uImage;					\
			mv u-boot.old u-boot.img;				\
			mv spondoolies.dtb.old spondoolies.dtb;			\
			exit 222; }

	mv -f ${curdir}/spondoolies.dtb spondoolies.dtb ||
		{ echo 'Cannot copy new DTB, revert to the old version';	\
			mv uImage.old uImage;					\
			mv u-boot.old u-boot.img;				\
			mv spondoolies.dtb.old spondoolies.dtb;			\
			exit 223; }

	sync
}



# NOTE: call this function in main instead of everything else
#	if the latest version already installed.
latest_version()
{
	echo 'The latest version already installed. No need to upgrade.'
	return 1
}

main()
{
	# NOTE: uncomment this line and comment everything else if
	#	the latest version already installed.
	#latest_version

	curdir=${PWD}
	get_new_version
	print_msg_start
	do_upgrade	
	print_msg_end
}

main $@
