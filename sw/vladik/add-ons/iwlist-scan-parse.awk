#!/usr/bin/awk -f

# Parse iwlist <iface> scan results
# Written by Vladik Goytin


BEGIN	{
		cell = 0
		FS=":"
	}

# Cell entry start.
/Cell/			{
				cell++
			}

/Address/		{
				gsub(/.*Address: /,"") 
				mac[cell] = $0
			}

/ESSID/			{
				essid[cell] = $2
			}

/Mode/			{
				mode[cell] = $2
			}

/Channel:/		{
				channel[cell] = $2
			}

/Quality/		{
				gsub(/.*Quality=/,"") 
				gsub(/ .*/,"") 
				quality[cell] = $1
			}

/Encryption/		{
				gsub(/.*Encryption key:/,"")
				encryption[cell] = $0
			}		

/WPA Version 1/		{
				# Always prefer WPA2 over WPA.
				if (proto[cell] != "WPA2")
					proto[cell] = "WPA"
			}

/IEEE 802.11i/		{
				proto[cell] = "WPA2"
			}

/Group Cipher/		{
				gsub(/.*Group Cipher : /,"")
				group[cell] = $0
			}

/Pairwise Ciphers/	{
				gsub(/.*Pairwise Ciphers.*: /,"")
				pairwise[cell] = $0
			}

/Authentication Suites/	{
				gsub(/.*Authentication Suites.*: /,"")
				key_mgmt[cell] = $0
			}


END	{
		max_cells = cell
		for(cell = 1; cell <= max_cells; cell++)
			if (mode[cell] == "Master")
			{
				printf "ESSID=%s MAC=%s Chan=%s Quality=%s Enc=%s ",
					essid[cell], mac[cell], channel[cell],
					quality[cell], encryption[cell]
				if (encryption[cell] == "on")
				{
					printf "Proto=%s Group=<%s> Pairwise=\"%s\" KeyMgmt=%s\n",
					proto[cell], group[cell], pairwise[cell], key_mgmt[cell]
				}
				else
					printf "\n"
			}
	}
