#!/bin/sh

# Download a file
#
# Written by Vladik Goytin

__prog_name=`basename $0`

url=
file=
query=


usage()
{
	cat<<-EOF
	Download a file
	Written by Vladik Goytin

	Usage: ${__prog_name} --help | --url=<URL> [--query=<GET query>]

	 	--help		-- show this help
	 	--url		-- file's URL to download
	 	--query		-- GET query

	Examples:
	 	${__prog_name} --url=https://mydomain.com/thefile --query="par1=val1&par2=val2"
	 		-- download file from URL https://mydomain.com/thefile
	EOF
}



parse_args()
{
	opts="help,url:,query:"
	temp=`getopt -o h --long ${opts} -n download-file.sh -- $@`
	[ $? -ne 0 ] && usage

	eval set -- "$temp"

	while :
	do
		case $1 in
		-h|--help)              usage; exit 0; shift ;;
		--url)			url=$2;
					file=`basename ${url}`
					shift 2 ;;
		--query)		query="?$2"; shift 2 ;;
		--)                     shift; break 2 ;;  # exit loop
		* )                     echo "unknown parameter $1"; return 1 ;;
	esac
	done

	[ x${url} != x ]
}


download_file()
{
	curl --fail --connect-timeout 20 --output ${file} -L "${url}${query}"
}

main()
{
	parse_args $@ || { usage; exit 1; }
	download_file
}

main $@
