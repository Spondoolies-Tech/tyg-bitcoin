#!/bin/sh

# Sign digital digest (SHA256) of a file.
#
# Written by Vladik Goytin

__prog_name=`basename $0`

file=
private=


usage()
{
	cat<<-EOF
	Sign digital digest (SHA256) of a file.
	Written by Vladik Goytin

	Usage: ${__prog_name} --help | --file=<input file> --private=<private key>

	 	--help		-- show this help
	 	--file		-- file which digital digest should signed.
	 	--private	-- private key used for signing.

	Examples:
	 	${__prog_name} --file=myfile --private=mykey.priv
	 		-- sign digital digest (SHA256) of file myfile using
	 		   private key mykey.priv
	EOF
}



parse_args()
{
	opts="help,file:,private:"
	temp=`getopt -o h --long ${opts} -n sign-digest.sh -- $@`
	[ $? -ne 0 ] && usage

	eval set -- "$temp"

	while :
	do
		case $1 in
		-h|--help)              usage; exit 0; shift ;;
		--file)			file=$2; shift 2 ;;
		--private)		private=$2; shift 2 ;;
		--)                     shift; break 2 ;;  # exit loop
		* )                     echo "unknown parameter $1"; return 1 ;;
	esac
	done

	[ x${file} != x ] && [ x${private} != x ]
}


sign_digest()
{
	openssl dgst -sha256 -sign ${private} -out ${file}.sign ${file}
}

main()
{
	parse_args $@ || { usage; exit 1; }
	sign_digest
}

main $@
