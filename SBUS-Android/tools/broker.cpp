// broker.cpp - DMI - 29-6-2007

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "../library/sbus.h"

int port = -1, bg = 0;

void mainloop();

void usage()
{
	printf("Usage: broker [-bg] [port-num]\n");
	printf("       -bg = run in background\n");
	exit(0);
}

void parse_args(int argc, char **argv)
{
	for(int i = 1; i < argc; i++)
	{
		if(!strcmp(argv[i], "-bg"))
			bg = 1;
		else if(argv[i][0] == '-')
			usage();
		else
		{
			if(port != -1)
				usage();
			port = atoi(argv[1]);
			if(port < 1 || port > 65535)
				usage();
		}
	}
}

int main(int argc, char **argv)
{
	parse_args(argc, argv);
	if(bg)
	{
		int fd;
		
		if(fork() > 0) exit(0); // Detach
		fd = open("/dev/null", O_RDWR);
		if(fd < 0) error("Cannot open /dev/null");
		if(close(STDIN_FILENO) < 0) error("Cannot close stdin");
		if(close(STDOUT_FILENO) < 0) error("Cannot close stdout");
		if(close(STDERR_FILENO) < 0) error("Cannot close stderr");
		dup2(fd, STDIN_FILENO);
		dup2(fd, STDOUT_FILENO);
		dup2(fd, STDERR_FILENO);
	}

	mainloop();	
	return 0;
}

void mainloop()
{
	const char *cpt_filename = "broker.cpt";
	int fd;
	multiplex *multi;
	sendpoint *ep;
	scomponent *com;
	sendpoint *notify_ep;
	
	com = new scomponent("broker");	
	com->add_endpoint("publish", EndpointSink, "FFFFFFFFFFFF");
	notify_ep = com->add_endpoint("notify", EndpointSource, "FFFFFFFFFFFF");
	
	com->start(cpt_filename, port);

	// Get FD's:
	multi = new multiplex();
	for(int i = 0; i < com->count_endpoints(); i++)
	{
		fd = com->get_endpoint(i)->fd;
		multi->add(fd, MULTI_READ);
	}
		
	// Select loop:
	while(1)
	{
		fd = multi->wait();
		if(fd < 0)
			continue;
		ep = com->fd_to_endpoint(fd);
		if(ep == NULL)
			continue;
		if(!strcmp(ep->name, "publish"))
		{
			smessage *msg;
			snode *sn;

			msg = ep->rcv();
			sn = msg->tree;
			printf("Received tree:\n");
			sn->dump();
			notify_ep->emit(sn, NULL, msg->hc);
			delete msg;	
		}
	}
	
	delete multi;
	delete com;
}
