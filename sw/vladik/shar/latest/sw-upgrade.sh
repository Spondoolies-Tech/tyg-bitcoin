#!/bin/sh
#
# Written by Vladik Goytin

latest_version()
{
	echo 'The latest Spondoolies software already installed.'
}

main()
{
	latest_version
	return 1
}

main $@
