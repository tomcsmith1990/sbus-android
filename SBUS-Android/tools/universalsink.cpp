// universalsink.cpp - DMI - 17-10-2007

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "../library/sbus.h"

const char *address, *endpoint, *subscription, *topic, *instance;
int xmlout;
int unmapped;

// Prototypes:
void do_sink(sendpoint *ep);

void usage()
{
	printf("Usage: universalsink [Options] <address> <endpoint> "
			 "[<instance-name>]\n");
	printf("       universalsink [-x] -u [<instance-name>]     Start unmapped\n");
	printf("Options: -t <topic>\n");
	printf("         -s <subscription>\n");
	printf("         -x     XML output\n");
	exit(0);
}

void parse_args(int argc, char **argv)
{
	subscription = topic = address = endpoint = instance = NULL;
	xmlout = unmapped = 0;
	
	if(argc < 2)
		usage();
	
	for(int i = 1; i < argc; i++)
	{
		if(argv[i][0] == '-')
		{
			if(!strcmp(argv[i], "-s"))
			{
				if(i == argc - 1)
					usage();
				i++;
				subscription = argv[i];
			}
			else if(!strcmp(argv[i], "-t"))
			{
				if(i == argc - 1)
					usage();
				i++;
				topic = argv[i];
			}
			else if(!strcmp(argv[i], "-x"))
				xmlout = 1;
			else if(!strcmp(argv[i], "-u"))
				unmapped = 1;
			else
				usage();
		}
		else
		{
			if(address == NULL && !unmapped)
				address = argv[i];
			else if(endpoint == NULL && !unmapped)
				endpoint = argv[i];
			else if(instance == NULL)
				instance = argv[i];
			else
				usage();
		}
	}
	if(!unmapped && (address == NULL || endpoint == NULL))
		usage();
}

int main(int argc, char **argv)
{
	const char *cpt_filename = "universalsink.cpt";
	scomponent *com;
	sendpoint *ep, *sink_ep;
	int fd;
	char *official_address;
	multiplex *multi;
	
	parse_args(argc, argv);
	scomponent::set_log_level(LogErrors | LogWarnings | LogMessages,
			LogErrors | LogWarnings);
	com = new scomponent("universalsink", instance);
	sink_ep = com->add_endpoint("sink", EndpointSink, "FFFFFFFFFFFF");
	
	com->start(cpt_filename);
	if(subscription != NULL || topic != NULL)
		sink_ep->subscribe(topic, subscription, NULL);
	
	// sink_ep->subscribe(NULL, "speed < 10", NULL);

	if(!unmapped)
	{
		official_address = sink_ep->map(address, endpoint);
		if(official_address == NULL)
		{
			printf("Failed to map address '%s', endpoint '%s'\n", address, endpoint);
			delete com;
			exit(0);
		}
		delete[] official_address;
	}
	
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
		if(!strcmp(ep->name, "sink"))
			do_sink(ep);
	}
	
	delete multi;
	delete com;
	return 0;
}

void do_sink(sendpoint *ep)
{
	smessage *msg;
	snode *sn;
	
	msg = ep->rcv();
	sn = msg->tree;
	if(xmlout)
	{
		const char *s = sn->toxml(1);
		printf("%s\n", s);
		delete[] s;
	}
	else
		sn->dump();
	delete msg;
}
