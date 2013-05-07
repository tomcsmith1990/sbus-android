// universalsource.cpp - DMI - 16-11-2008

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "../library/sbus.h"

const char *schemafile, *instance;

// Prototypes:
void do_sink(sendpoint *ep);

void usage()
{
	printf("Usage: universalsource <schema-file> [<instance-name>]\n");
	exit(0);
}

void parse_args(int argc, char **argv)
{
	instance = NULL;
	
	if(argc < 2 || argc > 3)
		usage();
	
	schemafile = argv[1];
	if(argc == 3)
		instance = argv[2];
}

void mainloop(scomponent *com, sendpoint *ep)
{
	StringBuf *sb;
	snode *sn;
	const char *err;
	size_t n = 0;
	ssize_t bytes;
	char *line = NULL;

	sb = new StringBuf();
	while(1)
	{
		bytes = getline(&line, &n, stdin);
		if(bytes < 0)
			break;
		if(bytes > 0)
		{
			if(line[0] == '\n' && !(sb->iswhitespace()))
			{
				sn = snode::import((const char *)sb->peek(), &err);
				if(sn == NULL)
					printf("Invalid XML: %s\n", err);
				else
				{
					ep->emit(sn);
					delete sn;
				}
				sb->clear();
			}
			else
				sb->cat(line);
		}
	}
	free(line);
	delete sb;
}

int main(int argc, char **argv)
{
	const char *cpt_filename = "universalsource.cpt";
	scomponent *com;
	sendpoint *ep;
	HashCode *hc;
	const char *schema;
	
	parse_args(argc, argv);
	scomponent::set_log_level(LogErrors | LogWarnings | LogMessages,
			LogErrors | LogWarnings);
	com = new scomponent("universalsource", instance);	
	com->start(cpt_filename);

	hc = com->load_schema(schemafile);
	ep = com->add_endpoint("source", EndpointSource, hc);
	schema = com->get_schema(hc);
	fprintf(stderr, "Using schema:\n%s\n", schema);
	delete[] schema;
	mainloop(com, ep);
	
	delete hc;
	delete com;
	return 0;
}

void do_sink(sendpoint *ep)
{
	smessage *msg;
	snode *sn;
	
	msg = ep->rcv();
	sn = msg->tree;
	sn->dump();
	delete msg;
}
