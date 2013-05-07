// slowcar.cpp - DMI - 17-10-2007

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "../library/sbus.h"

char *instance_name;

// Prototypes:
void do_traffic(sendpoint *ep);

void usage()
{
	printf("Usage: slowcar [instance-name]\n");
	exit(0);
}

void parse_args(int argc, char **argv)
{
	instance_name = NULL;
	
	if(argc > 2)
		usage();
	if(argc == 2)
	{
		if(argv[1][0] == '-')
			usage();
		instance_name = argv[1];
	}
}

int main(int argc, char **argv)
{
	const char *cpt_filename = "slowcar.cpt";
	scomponent *com;
	sendpoint *ep, *traffic_ep;
	int fd;
	multiplex *multi;
	
	parse_args(argc, argv);
	com = new scomponent("slowcar", instance_name);
	traffic_ep = com->add_endpoint("traffic", EndpointSink, "727AC14DAC71");
	
	com->start(cpt_filename);
	traffic_ep->subscribe(NULL, "speed < 10", NULL);

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
		if(!strcmp(ep->name, "traffic"))
			do_traffic(ep);
	}
	
	delete multi;
	delete com;
	return 0;
}

void do_traffic(sendpoint *ep)
{
	smessage *msg;
	const char *colour;
	int speed;
	
	msg = ep->rcv();
	colour = msg->tree->extract_txt("colour");
	speed = msg->tree->extract_int("speed");
	printf("Car speed %2d, colour %s\n", speed, colour);
	delete msg;
}
