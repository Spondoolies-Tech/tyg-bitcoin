#!/bin/sh

# Verify signed digital digest (SHA256) of a file.
#
# Written by Vladik Goytin

__prog_name=`basename $0`

file=
private=


usage()
{
	cat<<-EOF
	Verify signed digital digest (SHA256) of a file.
	Written by Vladik Goytin

	Usage: ${__prog_name} --help | --file=<input file> --public=<public key>

	 	--help		-- show this help
	 	--file		-- file which signed digital digest should be verified.
	 	--public	-- public key used for signing.

	Examples:
	 	${__prog_name} --file=myfile --public=mykey.pub
	 		-- sign SHA256 digital digest of file myfile using
	 		   private key mykey.pub
	EOF
}


parse_args()
{
	opts="help,file:,public:"
	temp=`getopt -o h --long ${opts} -n verify-digest.sh -- $@`
	[ $? -ne 0 ] && usage

	eval set -- "$temp"

	while :
	do
		case $1 in
		-h|--help)              usage; exit 0; shift ;;
		--file)			file=$2; shift 2 ;;
		--public)		public=$2; shift 2 ;;
		--)                     shift; break 2 ;;  # exit loop
		* )                     echo "unknown parameter $1"; return 1 ;;
	esac
	done

	[ x${file} != x ] && [ x${public} != x ]
}


verify_digest()
{
	openssl dgst -sha256 -verify ${public} -signature ${file}.sign ${file} >/dev/null 2>&1
}

main()
{
	parse_args $@ || { usage; exit 1; }
	verify_digest
}

main $@
