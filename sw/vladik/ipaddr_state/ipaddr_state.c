/* Written by Vladik Goytin.
 * Based on the code from here: http://stackoverflow.com/a/2353441
 */

#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <net/if.h>


#define MAX_MSG_SIZE		4096

/* No need to look at this interface. */
#define AVOIDED_IFACE		"lo"

static int			sock;


static int open_netlink_sock(void)
{
	int			rc;

	sock = socket(PF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
	if (sock >= 0)
	{
		static const struct sockaddr_nl addr = {
			nl_family:	AF_NETLINK,
			nl_groups:	RTMGRP_IPV4_IFADDR
		};

		rc = bind(sock, (struct sockaddr *)&addr, sizeof(addr));
		if (rc < 0)
			perror("bind(AF_NETLINK)");
	}
	else
	{
		perror("socket(PF_NETLINK, NETLINK_ROUTE)");
		rc = -1;
	}

	return rc;
}


static void parse_nl_message(struct nlmsghdr *nlh)
{
	if (nlh->nlmsg_type == RTM_NEWADDR || nlh->nlmsg_type == RTM_DELADDR)
	{
		struct ifaddrmsg *ifa = (struct ifaddrmsg *)NLMSG_DATA(nlh);
		struct rtattr	*rth = IFA_RTA(ifa);
		int rtl = IFA_PAYLOAD(nlh);

		while (rtl && RTA_OK(rth, rtl))
		{
			if (rth->rta_type == IFA_LOCAL)
			{
				char name[IFNAMSIZ];
				if_indextoname(ifa->ifa_index, name);

				if (strcmp(name, AVOIDED_IFACE))
				{
					printf("%s-%s\n", name,
						(nlh->nlmsg_type == RTM_NEWADDR) ?
							"add" : "remove");
					fflush(stdout);
				}
			}
			rth = RTA_NEXT(rth, rtl);
		}
	}
}


static int listen_nl(void)
{
	char			buf[MAX_MSG_SIZE];
	struct nlmsghdr		*nlh = (struct nlmsghdr *)buf;
	int			rc;

	while ((rc = recv(sock, nlh, MAX_MSG_SIZE, 0)) > 0)
	{
		while (NLMSG_OK(nlh, rc) && nlh->nlmsg_type != NLMSG_DONE)
		{
			parse_nl_message(nlh);
			nlh = NLMSG_NEXT(nlh, rc);
		}
	}

	return rc;
}


int
main()
{
	int			rc;

	rc = open_netlink_sock();
	if (rc == 0)
	{
		rc = listen_nl();
	}
	else
		fprintf(stderr, "Cannot operate NETLINK socket\n");

	return rc;
}
