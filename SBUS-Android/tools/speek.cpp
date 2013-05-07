// speek.cpp - DMI - 11-6-2007

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "../library/sbus.h"
#include "../wrapper/wrap.h"

char *address = NULL, *endpoint = NULL;
int mode;

enum { MODE_METADATA, MODE_STATUS, MODE_ENDPOINT, MODE_CONNECT,
		 MODE_SCHEMA, MODE_BUILTINS, MODE_BUILTIN };

const char *extract_option(snode *sn, const char *name);

void usage()
{
	printf("Usage: speek (-m | -s | -e | -c) <address>\n");
	printf("       speek schema <address> <endpoint>\n");
	printf("       speek builtin <endpoint>\n");
	printf("       speek builtins\n");
	printf("       -m = metadata\n");
	printf("       -s = status\n");
	printf("       -e = endpoint info\n");
	printf("       -c = connection info\n");
	exit(0);
}

void checkargs(int argc, int n)
{
	if(argc != n)
		usage();
}

void parse_args(int argc, char **argv)
{
	mode = 0;
	
	if(argc < 2)
		usage();
	
	if(!strcmp(argv[1], "-m"))
	{ mode = MODE_METADATA; checkargs(argc, 3); }
	else if(!strcmp(argv[1], "-s"))
	{ mode = MODE_STATUS; checkargs(argc, 3); }
	else if(!strcmp(argv[1], "-e"))
	{ mode = MODE_ENDPOINT; checkargs(argc, 3); }
	else if(!strcmp(argv[1], "-c"))
	{ mode = MODE_CONNECT; checkargs(argc, 3); }
	else if(!strcmp(argv[1], "schema"))
	{ mode = MODE_SCHEMA; checkargs(argc, 4); }
	else if(!strcmp(argv[1], "builtins"))
	{ mode = MODE_BUILTINS; checkargs(argc, 2); }
	else if(!strcmp(argv[1], "builtin"))
	{ mode = MODE_BUILTIN; checkargs(argc, 3); }
	else
		usage();
	
	if(mode == MODE_METADATA || mode == MODE_STATUS || mode == MODE_ENDPOINT
			|| mode == MODE_CONNECT || mode == MODE_SCHEMA)
		address = argv[2];
	if(mode == MODE_SCHEMA)
		endpoint = argv[3];
	if(mode == MODE_BUILTIN)
		endpoint = argv[2];
}

void print_params(const char *subs, const char *topic)
{
	if(subs != NULL && topic != NULL)
		printf("<%s>  \"%s\"\n", topic, subs);
	else if(topic != NULL)
		printf("<%s>  \"\"\n", topic);
	else if(subs != NULL)
		printf("any  \"%s\"\n", subs);
	else
		printf("any  \"\"\n");
}

const char *type_name(EndpointType t)
{
	switch(t)
	{
		case EndpointServer: return "server";
		case EndpointClient: return "client";
		case EndpointSource: return "source";
		case EndpointSink:   return "sink";
	}
	return "unknown";
}

int main(int argc, char **argv)
{
	const char *cpt_filename = "speek.cpt";
	scomponent *com;
	sendpoint *metadata_serv, *status_serv;
	smessage *reply;
	char *s;

	parse_args(argc, argv);
		
	scomponent::set_log_level(LogErrors | LogWarnings | LogMessages,
			LogErrors | LogWarnings);
	com = new scomponent("speek");
	
	metadata_serv = com->add_endpoint("get_metadata", EndpointClient,
			"000000000000", "6306677BFE43");
	status_serv = com->add_endpoint("get_status", EndpointClient,
			"000000000000", "253BAC1C33C7");

	com->start(cpt_filename,-1,false);


	if(address != NULL)
	{
		if(!metadata_serv->map(address, NULL, 0))
		{
			printf("Failed to map address %s\n", address);
			delete com;
			exit(0);
		}
		if(!status_serv->map(address, NULL, 0))
		{
			printf("Failed to map address %s\n", address);
			delete com;
			exit(0);
		}
	}

	if(mode == MODE_METADATA)
	{
		reply = metadata_serv->rpc(NULL);
		s = reply->tree->toxml(1);
		printf("%s\n", s);
		delete[] s;
		delete reply;
	}
	else if(mode == MODE_STATUS)
	{
		reply = status_serv->rpc(NULL);
		s = reply->tree->toxml(1);
		printf("%s\n", s);
		delete[] s;
		delete reply;
	}
	else if(mode == MODE_BUILTINS)
	{
		int n;
		sbuiltin *sb;
		const char *type;
		
		n = com->count_builtins();
		for(int i = 0; i < n; i++)
		{
			sb = com->get_builtin(i);
			type = type_name(sb->type);
			printf("%2d. %-16s %-6s\n", i + 1, sb->name, type);
		}
	}
	else if(mode == MODE_BUILTIN)
	{
		HashCode *msg_hc = NULL, *reply_hc = NULL;
		int n;
		sbuiltin *sb;
		const char *schema;
		
		n = com->count_builtins();
		for(int i = 0; i < n; i++)
		{
			sb = com->get_builtin(i);
			if(!strcmp(sb->name, endpoint))
			{
				msg_hc = sb->msg_hc;
				reply_hc = sb->reply_hc;
				break;
			}
		}
		if(msg_hc == NULL)
			error("No such builtin endpoint.");
		schema = com->get_schema(msg_hc);
		if(strcmp(schema, "0"))
			printf("%s\n", schema);
		else
			printf("Empty message\n");
		delete[] schema;
		schema = com->get_schema(reply_hc);
		if(strcmp(schema, "!"))
			printf("Response:\n%s\n", schema);
		delete[] schema;
	}
	else if(mode == MODE_SCHEMA)
	{
		snode *sn, *subn;
		const char *name, *message, *response;
		Schema *sch;
		const char *err;
		
		reply = metadata_serv->rpc(NULL);
		sn = reply->tree->extract_item("endpoints");
		for(int i = 0; i < sn->count(); i++)
		{
			subn = sn->extract_item(i);
			name = subn->extract_txt("name");
			if(!strcmp(name, endpoint))
			{
				message = subn->extract_txt("message");
				response = subn->extract_txt("response");
				sch = Schema::create(message, &err);
				if(sch == NULL)
					error("Invalid schema: %s", err);
				s = sch->canonical_string();
				if(strcmp(s, "0"))
					printf("%s", s);
				else
					printf("Empty message\n");
				delete[] s;
				delete sch;
				if(strcmp(response, "!"))
				{
					sch = Schema::create(response, &err);
					if(sch == NULL)
						error("Invalid schema: %s", err);
					s = sch->canonical_string();
					printf("Response:\n%s", s);
					delete[] s;
					delete sch;
				}
				break;
			}
		}
		delete reply;
	}
	else if(mode == MODE_ENDPOINT || mode == MODE_CONNECT)
	{
		smessage *status_reply, *metadata_reply;
		snode *sn_status, *sn_metadata, *subn_status, *subn_metadata;
		snode *sn_peers, *sn_peer;
		int num_eps, num_builtins, num_custom;
		const char *status_name, *metadata_name, *type;
		const char *subs, *topic;
		const char *peer_name, *peer_instance;
		int processed, num_peers;
		
		metadata_reply = metadata_serv->rpc(NULL);
		status_reply = status_serv->rpc(NULL);
		
		sn_metadata = metadata_reply->tree->extract_item("endpoints");
		sn_status = status_reply->tree->extract_item("endpoints");
		num_eps = sn_status->count();
		num_custom = sn_metadata->count();
		num_builtins = com->count_builtins();
		if(num_eps != num_custom + num_builtins)
		{
			error("Different number of endpoints returned by status/metadata"
					" (%d vs %d + %d)", num_eps, num_custom, num_builtins);
		}
		
		printf("%d endpoints: %d application, %d builtin\n", num_eps,
				num_custom, num_builtins);
		
		for(int i = 0; i < num_custom; i++)
		{
			subn_metadata = sn_metadata->extract_item(i);
			metadata_name = subn_metadata->extract_txt("name");
		
			subn_status = sn_status->extract_item(i + num_builtins);
			status_name = subn_status->extract_txt("name");
			
			if(strcmp(metadata_name, status_name) != 0)
			{
				error("Endpoints have different names in component metadata "
						"and status structures");
			}
			
			type = subn_metadata->extract_value("type");
			processed = subn_status->extract_int("processed");
			subs = extract_option(subn_status, "subscription");
			topic = extract_option(subn_status, "topic");
			sn_peers = subn_status->extract_item("peers");
			num_peers = sn_peers->count();

			if(mode == MODE_ENDPOINT)
			{				
				printf("%-16s %-6s  maps %d, msgs %d %s\n",
						metadata_name, type, num_peers, processed,
						(subs == NULL ? "" : "[S]"));
			}
			else
			{
				printf("%-16s %-6s  ", metadata_name, type);
				print_params(subs, topic);
				
				for(int j = 0; j < num_peers; j++)
				{
					sn_peer = sn_peers->extract_item(j);
					peer_name = sn_peer->extract_txt("cpt_name");
					peer_instance = sn_peer->extract_txt("instance");

					subs = extract_option(sn_peer, "subscription");					
					topic = extract_option(sn_peer, "topic");
					
					if(peer_instance && strcmp(peer_name, peer_instance) != 0)
					{
						char *s = new char[strlen(peer_name) +
								strlen(peer_instance) + 10];
						sprintf(s, "%s(%s)", peer_name, peer_instance);
						printf("   %-20s  ", s);
						delete[] s;
					}
					else
						printf("   %-20s  ", peer_name);
					print_params(subs, topic);
				}
			}
		}
		
		delete metadata_reply;
		delete status_reply;
	}

	if(address != NULL)
	{	
		metadata_serv->unmap();
		status_serv->unmap();
	}

	delete com;
	return 0;
}

const char *extract_option(snode *sn, const char *name)
{
	if(sn->exists(name))
		return sn->extract_txt(name);
	else
		return NULL;
}
