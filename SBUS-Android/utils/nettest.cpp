// nettest.cpp - DMI - 26-5-2007

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <math.h>
#include <signal.h>
#include <errno.h>

#include "../library/sbus.h"
#include "../library/lowlevel.h"
#include "../wrapper/wrap.h"

void receive()
{
	int sock, master_sock, port = -1, bytes, remaining;
	char *buf, *pos;
	
	signal(SIGPIPE, SIG_IGN);
	buf = new char[100];
	master_sock = passivesock(&port);
	printf("Listening on port %d\n", port);
	sock = acceptsock(master_sock);
	sock_noblock(sock);
	printf("Incoming connection\n");
	remaining = 90;
	pos = buf;
	while(remaining > 0)
	{
		bytes = read(sock, pos, remaining);
		if(bytes == 0)
			error("Encountered end-of-file on read");
		if(bytes < 0)
		{
			if(errno == EAGAIN)
			{
				printf(".");
				continue;
			}
			error("Error on read");
		}
		printf("Read %d bytes\n", bytes);
		remaining -= bytes;
		pos += bytes;
	}
	close(sock);
	close(master_sock);
	delete[] buf;
}

void send(int port)
{
	char *buf;
	int sock, bytes;
	
	signal(SIGPIPE, SIG_IGN);
	buf = new char[100];
	sock = activesock(port);
	sleep(3);
	bytes = write(sock, buf, 30);
	printf("Wrote %d bytes\n", bytes);
	sleep(3);
	bytes = write(sock, buf + 30, 30);
	printf("Wrote %d bytes\n", bytes);
	sleep(3);
	bytes = write(sock, buf + 60, 30);
	printf("Wrote %d bytes\n", bytes);
	close(sock);
	delete[] buf;
}

void usage()
{
	printf("Usage: nettest server\n");
	printf("       nettest <port #>\n");
	exit(0);
}

int main(int argc, char **argv)
{
	if(argc != 2)
		usage();
	if(!strcmp(argv[1], "server"))
		receive();
	else if(argv[1][0] == '-')
		usage();
	else
	{
		int port = atoi(argv[1]);
		if(port > 0 && port < 65536)
			send(port);
		else
			usage();
	}
	
	return 0;
}
