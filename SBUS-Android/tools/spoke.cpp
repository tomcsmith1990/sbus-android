// spoke.cpp - DMI - 16-6-2007

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "../library/sbus.h"

char *address, *endpoint, *toaddr, *toendpt, *divpeer, *divendpt;
char *subs, *topic, *peercpt;
char *schemafile, *schema2file, *xml;
int mode;
int log_lev, echo_lev;

enum { MODE_MAP, MODE_UNMAP, MODE_DIVERT, MODE_LOG, MODE_TERMINATE,
		MODE_SUBSCRIBE, MODE_TOPIC, MODE_EMIT, MODE_RPC, MODE_SEND };

void usage()
{
	printf("Usage: spoke map    <address> <endpoint> <to addr> <to endpt>\n");
	printf("       spoke unmap  <address> <endpoint>\n");
	printf("       spoke unmap  <address> <endpoint> <peer addr> <peer endpt>\n");
	printf("       spoke divert <address> <endpoint> <new addr> <new endpt> <peer addr> <peer endpt>\n");
	printf("       spoke divert <address> <endpoint> <new addr> <new endpt>\n");
	printf("       spoke subscribe <addr> <endpoint> <subs-expr> "
			"[<peer-cpt> | '*']\n");
	printf("       spoke filter <addr> <endpoint> <subs-expr> "
				"[<peer-cpt> | '*']\n");

	printf("       spoke topic  <address> <endpoint> <topic>     "
			"[<peer-cpt> | '*']\n");
	printf("       spoke emit   <address> <endpoint> <schema-file> [<XML>]\n");
	printf("       spoke rpc    <address> <endpoint> <schema-file> "
			"<schema-file> [<XML>]\n");
	printf("       spoke send   <address> <endpoint> [<XML>]\n");
	printf("       spoke log    <address> <log-level> <echo-level>\n");
	printf("       spoke terminate <address>\n");
	printf("Log levels: 1 = errors, 2 = warnings, 4 = messages, 8 = debug\n");
	exit(0);
}

char *arg_collate(char **argv, int n)
{
	StringBuf *sb;
	char *s;
	
	sb = new StringBuf();
	for(int i = 0; i < n; i++)
	{
		sb->cat(argv[i]);
		if(i != n - 1)
			sb->cat(' ');
	}
	s = sb->extract();
	delete sb;
	return s;
}

char *read_stdin()
{
	StringBuf *sb;
	char *s;
	char *line = NULL;
	size_t n = 0;
	ssize_t bytes;
	
	sb = new StringBuf();
	while(1)
	{
		bytes = getline(&line, &n, stdin);
		if(bytes < 0)
			break;
		if(bytes > 0)
			sb->cat(line);
	}
	free(line);
	s = sb->extract();
	delete sb;
	return s;
}

void parse_args(int argc, char **argv)
{
	if(argc < 3)
		usage();	
	address = argv[2];
	peercpt = NULL;
	
	if(!strcmp(argv[1], "map"))
	{
		mode = MODE_MAP;
		if(argc != 6) usage();
		endpoint = argv[3];
		toaddr = argv[4];
		toendpt = argv[5];
	}
	else if(!strcmp(argv[1], "unmap"))
	{
		mode = MODE_UNMAP;
		if (argc < 4 ||  argc >6) usage();
		endpoint = argv[3];
		if (argc>4)
			toaddr = argv[4];
		if (argc==6)
			toendpt = argv[5];	
	}
	else if(!strcmp(argv[1], "divert"))
	{
		mode = MODE_DIVERT;
		if(argc < 6 || argc > 8 ) usage();
		endpoint = argv[3];
		toaddr = argv[4];
		toendpt = argv[5];
		if (argc>6)
			divpeer = argv[6];
		if (argc==8)
			divendpt = argv[7];
	}
	else if(!strcmp(argv[1], "subscribe") | !strcmp(argv[1], "filter"))
	{
		mode = MODE_SUBSCRIBE;
		if(argc < 5 || argc > 6) usage();
		endpoint = argv[3];
		subs = argv[4];
		topic = NULL;
		if(argc == 6)
			peercpt = argv[5];
	}
	else if(!strcmp(argv[1], "topic"))
	{
		mode = MODE_TOPIC;
		if(argc < 5 || argc > 6) usage();
		endpoint = argv[3];
		subs = NULL;
		topic = argv[4];
		if(argc == 6)
			peercpt = argv[5];
	}
	else if(!strcmp(argv[1], "emit"))
	{
		mode = MODE_EMIT;
		if(argc < 5) usage();
		endpoint = argv[3];
		schemafile = argv[4];
		if(argc > 5)
			xml = arg_collate(argv + 5, argc - 5);
		else
			xml = read_stdin();
	}
	else if(!strcmp(argv[1], "rpc"))
	{
		mode = MODE_RPC;
		if(argc < 6) usage();
		endpoint = argv[3];
		schemafile = argv[4];
		schema2file = argv[5];
		if(argc > 6)
			xml = arg_collate(argv + 6, argc - 6);
		else
			xml = read_stdin();
	}
	else if(!strcmp(argv[1], "send"))
	{
		mode = MODE_SEND;
		if(argc < 4) usage();
		endpoint = argv[3];
		if(argc > 4)
			xml = arg_collate(argv + 4, argc - 4);
		else
			xml = read_stdin();
	}
	else if(!strcmp(argv[1], "log"))
	{
		mode = MODE_LOG;
		if(argc != 5) usage();
		log_lev = atoi(argv[3]);
		echo_lev = atoi(argv[4]);
	}
	else if(!strcmp(argv[1], "terminate"))
	{
		mode = MODE_TERMINATE;
		if(argc != 3) usage();
	}
	else
		usage();
}

void map_failure(scomponent *com, const char *address)
{
	com->stop();
	printf("spoke: failed to map address %s\n", address);
	exit(0);
}

int isempty(const char *xml)
{
	while(1)
	{
		if(*xml == '\0')
			return 1;
		else if(*xml != ' ' && *xml != '\t' && *xml != '\n')
			return 0;
		xml++;
	}
	return -1; // Never occurs
}

int main(int argc, char **argv)
{
	const char *cpt_filename = "spoke.cpt";
	scomponent *com;
	sendpoint *map_ep, *unmap_ep, *divert_ep, *log_ep, *terminate_ep, *subs_ep;
	sendpoint *metadata_ep; // For send mode
	snode *sn, *sn_endpt, *sn_toaddr, *sn_toendpt, *sn_cert, *sn_divpeer, *sn_divendpt;
	snode *sn_log, *sn_echo, *sn_peer, *sn_subs, *sn_topic;

	parse_args(argc, argv);
		
	scomponent::set_log_level(LogErrors | LogWarnings | LogMessages,
			LogErrors | LogWarnings);
	
	com = new scomponent("spoke");
	
	map_ep = com->add_endpoint("map", EndpointSource, "F46B9113DB2D");
	unmap_ep = com->add_endpoint("unmap", EndpointSource, "FCEDAD0B6FE1");
	divert_ep = com->add_endpoint("divert", EndpointSource, "C648D3D07AE8");
	subs_ep = com->add_endpoint("subscribe", EndpointSource, "72904AC06922");
	log_ep = com->add_endpoint("set_log_level", EndpointSource, "D8A30E59C04A");
	terminate_ep = com->add_endpoint("terminate", EndpointSource,
			"000000000000");
	metadata_ep = com->add_endpoint("get_metadata", EndpointClient,
			"000000000000", "6306677BFE43");

	com->start(cpt_filename,-1,false);

	sn_cert = pack("", "certificate");
	sn_endpt = pack(endpoint, "endpoint");
	
	if(mode == MODE_MAP)
	{
		if(!map_ep->map(address, NULL))
			map_failure(com, address);
		sn_toaddr = pack(toaddr, "peer_address");
		sn_toendpt = pack(toendpt, "peer_endpoint");
		sn = pack(sn_endpt, sn_toaddr, sn_toendpt, sn_cert, "map");
		map_ep->emit(sn);
		map_ep->unmap();
	}
	if(mode == MODE_UNMAP)
	{
		if(!unmap_ep->map(address, NULL))
			map_failure(com, address);
	
		if (toaddr==NULL || !strcmp(toaddr,"NULL"))
			sn_toaddr= pack(SNull, "peer_address");
		else
			sn_toaddr = pack(toaddr, "peer_address");
		if (toendpt==NULL || !strcmp(toendpt,"NULL"))
			sn_toendpt = pack(SNull, "peer_endpoint");
		else
			sn_toendpt = pack(toendpt, "peer_endpoint");
	
		sn = pack(sn_endpt, sn_toaddr, sn_toendpt, sn_cert, "unmap");
		unmap_ep->emit(sn);
		unmap_ep->unmap();
	}
	if(mode == MODE_DIVERT)
	{
		if(!divert_ep->map(address, NULL))
			map_failure(com, address);		
		sn_toaddr = pack(toaddr, "new_address");
		sn_toendpt = pack(toendpt, "new_endpoint");

		if (divpeer==NULL || !strcmp(divpeer,"NULL"))
			sn_divpeer= pack(SNull, "peer_address");
		else
			sn_divpeer = pack(divpeer, "peer_address");
		if (divendpt==NULL || !strcmp(divendpt,"NULL"))
			sn_divendpt = pack(SNull, "peer_endpoint");
		else
			sn_divendpt = pack(divendpt, "peer_endpoint");
		
		sn = pack(sn_endpt, sn_toaddr, sn_toendpt, sn_divpeer, sn_divendpt, sn_cert, "divert");
		divert_ep->emit(sn);
		divert_ep->unmap();
	}
	if(mode == MODE_EMIT)
	{
		HashCode *hc;
		sendpoint *ep;
		const char *err;

		// Uses address, endpoint, schemafile and xml		
		
		hc = com->load_schema(schemafile);
		ep = com->add_endpoint("emit", EndpointSource, hc);
		delete hc;
		if(!(ep->map(address, endpoint)))
			map_failure(com, address);
		if(isempty(xml))
			sn = NULL;
		else
		{
			sn = snode::import(xml, &err);
			if(sn == NULL)
				error("Invalid XML: %s", err);
		}
		ep->emit(sn);
		sleep(2); //quick hack to ensure it doesn't exit before sending the schema. TODO: resolve properly
		ep->unmap();
		printf("Message succesfully sent\n%s\n",sn->toxml(1));
		if(sn != NULL)
			delete sn;
	}
	if(mode == MODE_RPC)
	{
		HashCode *msg_hc, *reply_hc;
		sendpoint *ep;
		const char *err;
		smessage *msg;
		const char *xmlout;

		// Uses address, endpoint, schemafile, schema2file and xml
		
		msg_hc = com->load_schema(schemafile);
		reply_hc = com->load_schema(schema2file);
		ep = com->add_endpoint("emit", EndpointClient, msg_hc, reply_hc);
		delete msg_hc;
		delete reply_hc;
		if(!(ep->map(address, endpoint)))
			map_failure(com, address);
		if(isempty(xml))
			sn = NULL;
		else
		{
			sn = snode::import(xml, &err);
			if(sn == NULL)
				error("Invalid XML: %s", err);
		}
		msg = ep->rpc(sn);
		ep->unmap();
		// msg->tree->dump();
		xmlout = msg->tree->toxml(1);
		printf("%s", xmlout);
		delete[] xmlout;
		delete msg;
		if(sn != NULL)
			delete sn;
	}
	if(mode == MODE_SEND)
	{
		// Uses address, endpoint and xml
		smessage *reply;
		const char *type, *name, *message, *response;
		snode *subn;
		int rpc;
		sendpoint *ep;
		HashCode *msg_hc, *reply_hc;
		const char *xmlout;
		const char *err;
		
		// First, discover type and schema[s] for target endpoint:
		if(!(metadata_ep->map(address, NULL)))
			map_failure(com, address);
		reply = metadata_ep->rpc(NULL);
		sn = reply->tree->extract_item("endpoints");
		message = NULL;
		for(int i = 0; i < sn->count(); i++)
		{
			subn = sn->extract_item(i);
			name = subn->extract_txt("name");
			printf("Comparing %s with %s\n",name,endpoint);
			if(!strcmp(name, endpoint))
			{
				type = subn->extract_value("type");
				message = subn->extract_txt("message");
				response = subn->extract_txt("response");
				break;
			}
		}
		if(message == NULL)
			error("No such endpoint provided by that component (or it may be a builtin, so send with EMIT and set the schema manually!).");
		metadata_ep->unmap();
		printf("Endpoint type %s\n", type);
		if(!strcmp(type, "server"))
			rpc = 1;
		else if(!strcmp(type, "sink"))
			rpc = 0;
		else
			error("Endpoint type %s invalid - must be server or sink.", type);
		
		// Now, create an appropriate client/source endpoint and send from it:
		msg_hc = com->declare_schema(message);
		if(rpc)
		{
			reply_hc = com->declare_schema(response);
			ep = com->add_endpoint("emit", EndpointClient, msg_hc, reply_hc);
			delete reply_hc;
		}
		else
			ep = com->add_endpoint("emit", EndpointSource, msg_hc);
		delete msg_hc;
		delete reply;
		
		if(!(ep->map(address, endpoint)))
			map_failure(com, address);
		if(isempty(xml))
			sn = NULL;
		else
		{
			sn = snode::import(xml, &err);
			if(sn == NULL)
				error("Invalid XML: %s", err);
		}
		if(rpc)
		{
			reply = ep->rpc(sn);
			// reply->tree->dump();
			xmlout = reply->tree->toxml(1);
			printf("%s", xmlout);
			delete[] xmlout;
			delete reply;
		}
		else
		{
			ep->emit(sn);
		}
		ep->unmap();
		if(sn != NULL)
			delete sn;
	}
	if(mode == MODE_LOG)
	{
		if(!log_ep->map(address, NULL))
			map_failure(com, address);
		sn_log = pack(log_lev, "log");
		sn_echo = pack(echo_lev, "echo");
		sn = pack(sn_log, sn_echo, "level");
		log_ep->emit(sn);
		log_ep->unmap();
	}
	if(mode == MODE_TERMINATE)
	{
		if(!terminate_ep->map(address, NULL))
			map_failure(com, address);
		terminate_ep->emit(NULL);
		terminate_ep->unmap();
	}
	if(mode == MODE_SUBSCRIBE || mode == MODE_TOPIC)
	{
		if(!subs_ep->map(address, NULL))
			map_failure(com, address);
		sn_peer = pack(peercpt, "peer");
		sn_subs = pack(((subs == NULL) ? "" : subs), "subscription");
		sn_topic = pack(((topic == NULL) ? "" : topic), "topic");
		sn = pack(sn_endpt, sn_peer, sn_subs, sn_topic);
		subs_ep->emit(sn);
		subs_ep->unmap();
	}

	delete com;
	return 0;
}
