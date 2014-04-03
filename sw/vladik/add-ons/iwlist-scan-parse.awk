#!/usr/bin/awk -f

# Parse iwlist <iface> scan results and outputs it in JSON format.
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
				if ($0 == "on")
					encryption[cell] = "true"
				else
					encryption[cell] = "false"
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
				if (key_mgmt[cell] == "PSK" || key_mgmt[cell] == "EAP")
					key_mgmt[cell] = "WPA-" key_mgmt[cell]
			}


END	{
		max_cells = cell
		printf "{\n\"WiFi\": [\n"
		delimiter = ","

		for(cell = 1; cell <= max_cells; cell++)
		{
			# JSON dislikes the trailing "," so remove it for the last item.
			if (cell == max_cells)
				delimiter = ""

			if (mode[cell] == "Master")
			{
				printf "{ \"ESSID\": %s, \"MAC\": \"%s\", \"Chan\": %s, ",
					essid[cell], mac[cell], channel[cell]
				printf "\"Quality\": \"%s\", \"Enc\": %s ",
					quality[cell], encryption[cell]
				if (encryption[cell] == "true")
				{
					printf ", \"Proto\": \"%s\", \"Group\": \"%s\",	",
						proto[cell], group[cell]
					printf "\"Pairwise\": \"%s\", \"KeyMgmt\" :\"%s\" }%s\n",
						pairwise[cell], key_mgmt[cell], delimiter
				}
				else
					printf "}%s\n", delimiter
			}
		}
		printf "]}\n"
	}
