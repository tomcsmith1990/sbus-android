// democlient.cpp - DMI - 6-4-2007

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../library/sbus.h"

char *address = NULL;
int disrupt = 0;

void usage()
{
	printf("Usage: democlient [-disrupt] <address>\n");
//	printf("       [FUTURE:] democlient -u   [unmapped]\n");
	exit(0);
}

void parse_args(int argc, char **argv)
{
	if(argc < 2)
		usage();
	for(int i = 1; i < argc; i++)
	{
		if(!strcmp(argv[i], "-disrupt"))
			disrupt = 1;
		else if(argv[i][0] == '-')
			usage();
		else if(address != NULL)
			usage();
		else
			address = argv[i];
	}
	if(address == NULL)
		usage();
}

int main(int argc, char **argv)
{
	const char *cpt_filename = "democlient.cpt";
	scomponent *com;
	sendpoint *usage_serv, *log_serv;
	snode *sn, *sn2, *sn3;
	smessage *reply;

	parse_args(argc, argv);
		
	com = new scomponent("democlient");
	
	log_serv = com->add_endpoint("log", EndpointSource, "B732FBF25198");
	usage_serv = com->add_endpoint("usage", EndpointClient, "000000000000",
			"C50A50E9216F");

	com->start(cpt_filename);

	printf("Sending some messages to unmapped endpoint\n");
	sn = pack("TIME rocks");	
	sn2 = pack("TIME rolls");
	sn3 = pack("status?");
	log_serv->emit(sn);
	log_serv->emit(sn);
	
	printf("Mapping now\n");
	if(!log_serv->map(address, NULL, 0))
		error("Failed to map address %s", address);
	if(!usage_serv->map(address, NULL, 0))
		error("Failed to map address %s", address);
	printf("Maps completed successfully\n");
	
	reply = usage_serv->rpc(NULL);
	printf("Logged %d messages\n", reply->tree->extract_int("logged"));
	delete reply;
	
	printf("Sending various messages to mapped endpoint\n");
	log_serv->emit(sn2);
	
	log_serv->emit(sn);
	log_serv->emit(sn);
	log_serv->emit(sn, disrupt ? "disrupt" : NULL);
	log_serv->emit(sn);
	reply = usage_serv->rpc(NULL);
	printf("Logged %d messages\n", reply->tree->extract_int("logged"));
	delete reply;
	// sleep(5);
	reply = usage_serv->rpc(NULL);
	printf("Logged %d messages\n", reply->tree->extract_int("logged"));
	delete reply;

	log_serv->emit(sn3);
		
	printf("Unmapping now\n");
	log_serv->unmap();
	usage_serv->unmap();

	delete sn;
	delete com;
	return 0;
}
