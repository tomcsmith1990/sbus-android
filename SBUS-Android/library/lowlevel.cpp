// lowlevel.cpp - DMI - 9-5-2009

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>
#include <ifaddrs.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include "error.h"
#include "lowlevel.h"

int do_connect(const char *remote_hostname, int port, int flags);

void sock_nodelay(int sock)
{
	#ifndef __ANDROID__
	struct protoent *pe = getprotobyname("tcp");
	int opt_true = 1;
	setsockopt(sock, pe->p_proto, TCP_NODELAY, &opt_true, sizeof(int));
	#else
	int opt_true = 1;
	setsockopt(sock, 0, TCP_NODELAY, &opt_true, sizeof(int));
	#endif
}

void sock_noblock(int sock)
{
	fcntl(sock, F_SETFL, O_NONBLOCK);
}

void sock_reuseaddr(int sock)
{
	int opt_true = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt_true, sizeof(opt_true));
}

int fixed_write(int sock, const unsigned char *buf, int nbytes)
{
	int bytes;
	
	while(nbytes > 0)
	{
		bytes = write(sock, buf, nbytes);
		if(bytes < 1)
			return -1;
		nbytes -= bytes;
		buf += bytes;
	}
	return 0;
}

int fixed_read(int sock, unsigned char *buf, int nbytes)
{
	int remain = nbytes;
	unsigned char *pos = buf;
	int amount;
	
	while(remain > 0)
	{
		amount = read(sock, pos, remain);
		if(amount <= 0)
			return -1; // EOF or error
		remain -= amount;
		pos += amount;
	}
	return 0;
}

int passivesock(int *port)
{
	struct protoent *ppe;
	struct sockaddr_in local_addr;
	int sock;
	const int qlen = 4;
	
	memset(&local_addr, 0, sizeof(local_addr));
	local_addr.sin_family = AF_INET;
	local_addr.sin_addr.s_addr = INADDR_ANY;
	if(*port == -1)
		local_addr.sin_port = htons(0); // Any port will do
	else
		local_addr.sin_port = htons(*port);
		
	// Map protocol name to protocol number:
	#ifndef __ANDROID__
	if((ppe = getprotobyname("tcp")) == 0)
		error("Can't get tcp protocol entry");
	
	// Allocate a socket:
	sock = socket(PF_INET, SOCK_STREAM, ppe->p_proto);
	#else
	sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	#endif
	
	if(sock < 0)
		error("Can't create socket: %s", strerror(errno));
	sock_reuseaddr(sock);	
	
	// Bind this socket:
	if(bind(sock, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0)
	{
		if(*port == -1)
			error("Can't bind: %s", strerror(errno));
		else
			error("Can't bind to port %d: %s", *port, strerror(errno));
	}

	socklen_t size = sizeof(local_addr);	
	getsockname(sock, (struct sockaddr *)&local_addr, &size);
	*port = ntohs(local_addr.sin_port);
	// log("Stream source starting at port number %d", *port);
	
	if(listen(sock, qlen) < 0)
		error("Can't listen: %s", strerror(errno));

	return sock;
}

char *get_local_address(int sock)
{
	/* Methods for getting local IP address:
		Note there is no point asking for the local DNS name, since
		we may not have a DNS entry.
		
		- DNS method: man resolver - no good if no DNS entry.
		- getsockname() method: requires a remote socket; local ones
		  give 127.0.0.1 and we may not have a remote connection at
		  component start time.
		- Dummy socket method: connect it to "google.com", then use
		  getsockname() to get the address. This works but isn't
		  elegant.
		- gethostname() and gethostbyname() method: the hostent
		  structure appears to have a nice list of addresses, but
		  actually these calls are lazy and don't do lookups for the
		  local host, just returning whatever is in /etc/hosts,
		  not reporting interfaces, ignoring DHCP and calling it all
		  127.0.0.1 if that is mentioned in /etc/hosts.
		- ifconfig method: capture and grep ifconfig output
		  (very inelegant!)
		- IOCTL method: fine, but only works on Linux.
		- getifaddrs method: should be portable (Linux, FreeBSD, OSX).
		
		We use getifaddrs for the IP address, taking the first non-loopback
		interface as the canonical address (unless loopback is the only one).
		This then combined with the port number of the externally listening
		port.
	*/
	
	struct sockaddr_in local_addr;
	int addr_len;
	int ret;
	int local_port;
	char *address;
	char *dotted;
	
	addr_len = sizeof(local_addr);
	ret = getsockname(sock, (struct sockaddr *)(&local_addr),
			(socklen_t *)(&addr_len));
	if(ret < 0)
		error("Can't get socket name");
	local_port = ntohs(local_addr.sin_port);

	#ifndef __ANDROID__
	dotted = get_local_ip();
	#else
	dotted = get_local_ip_socket();	
	#endif

	address = new char[strlen(dotted) + 20];
	sprintf(address, "%s:%d", dotted, local_port);

	delete[] dotted;

	// printf("DBG> get_local_address: %s\n", address);	
	return address;
}
#ifndef __ANDROID__
char *get_local_ip()
{
	// ifaddrs method:
	const int buflen = 100;
	char *ip;
	socklen_t salen;
	char *interface;
	
	interface = getenv("SBUS_INTERFACE");
	
	ip = new char[buflen];
	struct ifaddrs *ifa = NULL, *ifp = NULL, *ifloopback = NULL;
	if(getifaddrs(&ifp) < 0)
		error("getifaddrs() failed");
	for(ifa = ifp; ifa; ifa = ifa->ifa_next)
	{
		if(ifa->ifa_addr->sa_family == AF_INET)
			salen = sizeof(struct sockaddr_in);
		else
			continue; // Skip this interface

		if(getnameinfo(ifa->ifa_addr, salen, ip, buflen, NULL, 0,
				NI_NUMERICHOST) < 0)
			continue; // Skip this interface

		if(interface != NULL && strcmp(ifa->ifa_name, interface) != 0)
			continue; // Not what we were asked for; skip it
				
		if(ifa->ifa_flags & IFF_LOOPBACK)
		{
			ifloopback = ifa;
			continue; // Skip loopback interface, unless it's the only one we have
		}
		break; // Success
	}
	if(ifa == NULL && ifloopback != NULL)
		ifa = ifloopback; // Last resort
	if(ifa == NULL)
	{
		if(interface != NULL)
			error("Required network interface %s does not exist", interface);
		else
			error("No network interfaces found (at least loopback must exist)");
	}
	debug("IFADDRS> %s (%s)\n", ip, ifa->ifa_name);
	freeifaddrs(ifp);
	
	return ip;
	
	/*
	The ifaddrs structure contains at least the following entries:

   struct ifaddrs   *ifa_next;         // Pointer to next struct
   char             *ifa_name;         // Interface name
   u_int             ifa_flags;        // Interface flags
   struct sockaddr  *ifa_addr;         // Interface address
   struct sockaddr  *ifa_netmask;      // Interface netmask
   struct sockaddr  *ifa_dstaddr;      // P2P interface destination
   void             *ifa_data;         // Address specific data
	*/
}
#endif

char *get_local_ip_socket()
{
	
	int sock;
    struct ifconf ifconf;
    struct ifreq ifreq[20];
    int interfaces;
 	char ip[INET_ADDRSTRLEN];
    char *address;
    address = new char[INET_ADDRSTRLEN];
 	
    // Create a socket or return an error.
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
       strcpy(ip, "127.0.0.1");
 
    // Point ifconf's ifc_buf to our array of interface ifreqs.
    ifconf.ifc_buf = (char *) ifreq;
    
    // Set ifconf's ifc_len to the length of our array of interface ifreqs.
    ifconf.ifc_len = sizeof ifreq;
 
    //  Populate ifconf.ifc_buf (ifreq) with a list of interface names and addresses.
    if (ioctl(sock, SIOCGIFCONF, &ifconf) == -1)
        strcpy(ip, "127.0.0.1");
 
    // Divide the length of the interface list by the size of each entry.
    // This gives us the number of interfaces on the system.
    interfaces = ifconf.ifc_len / sizeof(ifreq[0]);
    
    // Loop through the array of interfaces, printing each one's name and IP.
    for (int i = 0; i < interfaces; i++) {
        char net[INET_ADDRSTRLEN];
        struct sockaddr_in *address = (struct sockaddr_in *) &ifreq[i].ifr_addr;
 
        // Convert the binary IP address into a readable string.
        //inet_ntop(AF_INET, &address->sin_addr, net, sizeof(net));
 
        //printf("%s-%s\n", ifreq[i].ifr_name, net);
        
        if (!strcmp(ifreq[i].ifr_name, "lo") && ip == NULL)
        {
        	inet_ntop(AF_INET, &address->sin_addr, ip, sizeof(ip));
        }
        
        if (!strcmp(ifreq[i].ifr_name, "eth0"))
        {
        	inet_ntop(AF_INET, &address->sin_addr, ip, sizeof(ip));
        }
    }
 
    close(sock);
    
    if (ip != NULL)
    {
		// Copy it to a char *.
		for (int i = 0; i < INET_ADDRSTRLEN; i++)
			address[i] = ip[i];
	}
	else
	{
		strcpy(address, "127.0.0.1");
	}

    return address;
}

int acceptsock(int master_sock, char **remote_address)
{
	// Returns -1 on error
	int dyn_sock;
	struct sockaddr_in remote_addr;
	int addr_len;
	
	addr_len = sizeof(remote_addr);
	dyn_sock = accept(master_sock, (struct sockaddr *)(&remote_addr),
		(socklen_t *)(&addr_len));
	if(dyn_sock < 0)
	{
		// log("accept failed: %s", strerror(errno));
		return -1;
	}

	if(remote_address != NULL)
	{	
		const char *remote_ip_address;
		int remote_port;
		struct hostent *he;
		char *address;

		remote_ip_address = (char *)&(remote_addr.sin_addr);
		remote_port = ntohs(remote_addr.sin_port);	
		he = gethostbyaddr(remote_ip_address, 4, AF_INET);
		if(he == NULL)
			error("Can't gethostbyaddr() in acceptsock()");
		address = new char[strlen(he->h_name) + 20];
		sprintf(address, "%s:%d", he->h_name, remote_port);
		
		*remote_address = address;
	}
	
	sock_nodelay(dyn_sock);
	return dyn_sock;
}

int activesock(int port, int flags)
{
	return do_connect("localhost", port, flags);
}

int activesock(int port, const char *hostname, int flags)
{
	return do_connect(hostname, port, flags);
}

int activesock(const char *address, int flags)
{
	char *hostname = (char *)"localhost";
	int port = DEFAULT_PORT;
	int pos;
	int sock;
	int dealloc = 0;
	
	if(address == NULL)
		return -1;
	if(address[0] == ':')
	{
		// Port only:
		port = atoi(address + 1);
	}
	else
	{
		for(pos = 0; address[pos] != '\0' && address[pos] != ':'; pos++);
		
		if(address[pos] == '\0')
		{
			// Host only:
			hostname = (char *)address;
		}
		else
		{
			port = atoi(address + pos + 1);
			hostname = new char[pos + 1];
			strncpy(hostname, address, pos);
			hostname[pos] = '\0';
			dealloc = 1;
		}
	}
	
	sock = do_connect(hostname, port, flags);
	if(dealloc)
		delete[] hostname;
	return sock;
}

int do_connect(const char *remote_hostname, int port, int flags)
{
	struct hostent *phe;
	struct protoent *ppe;
	struct sockaddr_in remote_addr;
	int sock;

	memset(&remote_addr, 0, sizeof(remote_addr));
	remote_addr.sin_family = AF_INET;
	remote_addr.sin_port = htons(port);
	
	// Map host name to IP address, allowing for dotted decimal:
	if((phe = gethostbyname(remote_hostname)) != NULL)
	{
		memcpy(&remote_addr.sin_addr, phe->h_addr, phe->h_length);
	}
	else if((remote_addr.sin_addr.s_addr = inet_addr(remote_hostname))
		== INADDR_NONE)
	{
		if((flags & ACTIVE_SOCK_SILENT) == 0)
			warning("Can't get \"%s\" host entry", remote_hostname);
		return -1;
	}
	
	// Map transport protocol name to protocol number:
	#ifndef __ANDROID__
	if((ppe = getprotobyname("tcp")) == 0)
	{
		if((flags & ACTIVE_SOCK_SILENT) == 0)
			warning("Can't get tcp protocol entry");
		return -1;
	}
	
	// Allocate a socket:
	sock = socket(PF_INET, SOCK_STREAM, ppe->p_proto);
	#else
	sock = socket(PF_INET, SOCK_STREAM, 0);
	#endif
	
	if(sock < 0)
	{
		if((flags & ACTIVE_SOCK_SILENT) == 0)
			warning("Can't create socket: %s", strerror(errno));
		return -1;
	}
	sock_nodelay(sock);

	// XXX Never create a non-blocking socket for an outgoing connection
	// since the OS may not buffer subsequent writes to it.  Ideally will
	// add it to a queue for when the connect succeeeds.
	flags &= ~ACTIVE_SOCK_NONBLOCK;

	// Connect the socket:
	if(flags & ACTIVE_SOCK_NONBLOCK)
	{
		int ret;
		
		sock_noblock(sock);		
		ret = connect(sock, (struct sockaddr *)&remote_addr, sizeof(remote_addr));
		if(ret != -1)
			error("Non-blocking connect suspiciously did not return an error");
		if(errno != EINPROGRESS) // EINPROGRESS is normal return
		{
			if((flags & ACTIVE_SOCK_SILENT) == 0)
			{
				warning("Can't connect to <%s:%d> - %s", remote_hostname, port,
						strerror(errno));
			}
			return -1;
		}
	}
	else
	{
		if(connect(sock, (struct sockaddr *)&remote_addr, sizeof(remote_addr)) < 0)
		{
			if((flags & ACTIVE_SOCK_SILENT) == 0)
			{
				warning("Can't connect to <%s:%d> - %s", remote_hostname, port,
						strerror(errno));
			}
			return -1;
		}
	}
	
	return sock;
}
