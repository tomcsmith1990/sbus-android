// sbus.cpp - DMI - 25-6-2007

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "../library/sbus.h"

int mode;
char *rdc_address;
StringBuf *sb_cmd;

enum { MODE_LIST, MODE_DUMP, MODE_LINKS, MODE_HUP, MODE_REMOTE, MODE_LIST_DETAILED };

void usage()
{
	printf("Usage: sbus [option] list    	List known components\n");
	printf("       sbus [option] detailed   List known components and endpoints\n");
	printf("       sbus [option] dump    	Dump all info known by RDC\n");
	printf("       sbus [option] links   	Show links between components\n");
	printf("       sbus [option] hup    	Re-read persistent component list\n");
	printf("       sbus [option] start <command> <args...>   Remote start\n");
	printf("Option: -r <address>         Specify RDC address\n");
	exit(0);
}

void parse_args(int argc, char **argv)
{
	mode = -1;
	rdc_address = NULL;
	
	for(int i = 1; i < argc; i++)
	{
		if(!strcmp(argv[i], "list"))
			mode = MODE_LIST;
		else if(!strcmp(argv[i], "dump"))
			mode = MODE_DUMP;
		else if(!strcmp(argv[i], "links"))
			mode = MODE_LINKS;
		else if(!strcmp(argv[i], "hup"))
			mode = MODE_HUP;
		else if(!strcmp(argv[i], "start"))
		{
			mode = MODE_REMOTE;
			sb_cmd = new StringBuf();
			if(i == argc - 1)
				usage();
			i++;
			while(i < argc)
			{
				sb_cmd->cat(argv[i]);
				if(i < argc - 1)
					sb_cmd->cat(' ');
				i++;
			}
			break;
		}
		else if(!strcmp(argv[i], "-r"))
		{
			if(i == argc - 1)
				usage();
			i++;
			rdc_address = argv[i];
		}
		else if(!strcmp(argv[i], "detailed"))
				mode = MODE_LIST_DETAILED;
		else
			usage();
	}
	if(mode == -1)
		usage();
}

int main(int argc, char **argv)
{
	const char *cpt_filename = "sbus.cpt";
	scomponent *com;
	sendpoint *list_serv, *dump_serv, *hup_serv, *start_serv;
	smessage *reply;
	char *s;
	char *address;
	
	parse_args(argc, argv);
	if(rdc_address == NULL)
	{
		address = new char[50];
		sprintf(address, "localhost:%d", default_rdc_port);
	}
	else
		address = ipaddress::check_add_port(rdc_address, default_rdc_port);
			
	scomponent::set_log_level(LogErrors | LogWarnings | LogMessages,
			LogErrors | LogWarnings);
	com = new scomponent("sbus");
	
	list_serv = com->add_endpoint("list", EndpointClient,
			"000000000000", "46920F3551F9");
	dump_serv = com->add_endpoint("dump", EndpointClient,
			"000000000000", "534073C1E375");
	hup_serv = com->add_endpoint("hup", EndpointSource, "000000000000");
	start_serv = com->add_endpoint("remote_start", EndpointSource,
			"8F720B145518");

	com->start(cpt_filename,-1,false);

	if(mode == MODE_HUP)
	{
		if(!hup_serv->map(address, NULL, 0))
		{
			printf("Failed to map address %s\n", address);
			delete com;
			exit(0);
		}
		
		hup_serv->emit(NULL);
		
		hup_serv->unmap();
		delete com;
		return 0;
	}
	if(mode == MODE_REMOTE)
	{
		const char *cmd = sb_cmd->extract();
		snode *sn;
		
		if(!start_serv->map(address, NULL, 0))
		{
			printf("Failed to map address %s\n", address);
			delete cmd;
			delete sb_cmd;
			delete com;
			exit(0);
		}
		
		sn = pack(cmd, "cmdline");
		start_serv->emit(sn);
		
		start_serv->unmap();
		delete cmd;
		delete sb_cmd;
		delete com;
		return 0;
	}
	
	if(!list_serv->map(address, NULL, 0))
	{
		printf("Failed to map address %s\n", address);
		delete com;
		exit(0);
	}
	if(!dump_serv->map(address, NULL, 0))
	{
		printf("Failed to map address %s\n", address);
		delete com;
		exit(0);
	}

	if(mode == MODE_LIST)
	{
		snode *sn, *subn;
		const char *cpt, *instance, *addr;
		
		reply = list_serv->rpc(NULL);
		sn = reply->tree;
		printf("%d components:\n", sn->count());
		for(int i = 0; i < sn->count(); i++)
		{
			subn = sn->extract_item(i);
			cpt = subn->extract_txt("cpt-name");
			addr = subn->extract_txt("address");
			if(subn->exists("instance"))
			{
				instance = subn->extract_txt("instance");
				printf("cpt name: %-10s instance: %-10s addr: %s\n",
						cpt, instance, addr);
			}
			else
			{
				printf("cpt name: %-10s instance: %-10s addr: %s\n",
						cpt, "-", addr);
			}
		}
		delete reply;
	}
	else if(mode == MODE_DUMP)
	{
		reply = dump_serv->rpc(NULL);
		s = reply->tree->toxml(1);
		printf("%s\n", s);
		delete[] s;
		delete reply;
	}
	else if(mode == MODE_LINKS)
	{
		snode *sn_cpt, *sn_metadata, *sn_state, *sn_endpoints, *sn_peers;
		snode *sn;
		const char *cpt_name, *instance, *cpt_addr;
		const char *peer_cpt, *peer_inst, *peer_addr, *peer_endpt;
		int num_cpts, num_eps, num_peers;
		
		reply = dump_serv->rpc(NULL);
		num_cpts = reply->tree->count();
		printf("%d components:\n", num_cpts);
		for(int i = 0; i < num_cpts; i++)
		{
			sn_cpt = reply->tree->extract_item(i);
			sn_metadata = sn_cpt->extract_item("metadata");
			sn_state = sn_cpt->extract_item("state");
			
			cpt_name = sn_metadata->extract_txt("name");
			instance = sn_state->extract_txt("instance");
			cpt_addr = sn_cpt->extract_txt("address");
			if(!strcmp(cpt_name, instance))
				printf("cpt: %s (%s): \n", cpt_name, cpt_addr);
			else
				printf("cpt: %s/%s (%s): \n", cpt_name, instance, cpt_addr);
			
			sn_endpoints = sn_state->extract_item("endpoints");
			num_eps = sn_endpoints->count();
			for(int j = 0; j < num_eps; j++)
			{
				sn_peers = sn_endpoints->extract_item(j)->extract_item("peers");
				num_peers = sn_peers->count();
				if (num_peers > 0)
					printf("\t[%s]: ",sn_endpoints->extract_item(j)->extract_txt("name"));
				for(int k = 0; k < num_peers; k++)
				{
					sn = sn_peers->extract_item(k);
					peer_cpt = sn->extract_txt("cpt_name");
					peer_inst = sn->extract_txt("instance");
					peer_addr = sn->extract_txt("address");
					peer_endpt = sn->extract_txt("endpoint");
					
					if(strcmp(peer_cpt, peer_inst) == 0)
						printf("%s(%s[%s]); ", peer_cpt,peer_addr,peer_endpt);
					else
						printf("%s/%s(%s[%s]); ", peer_cpt, peer_inst,peer_addr,peer_endpt);
				
				}
				if (num_peers > 0)
						printf("\n");
			}
		}
		delete reply;
	}
	else if(mode == MODE_LIST_DETAILED)
	{
		snode *sn_cpt, *sn_metadata, *sn_state, *sn_endpoints, *sn_peers;
		snode *sn;
		const char *cpt_name, *instance, *cpt_addr;
		const char *peer_cpt, *peer_inst, *peer_addr, *peer_endpt;
		int num_cpts, num_eps, num_peers;
		svector *builtins;
		
		/* TODO - present has for each endpoint */
		reply = dump_serv->rpc(NULL);
		num_cpts = reply->tree->count();
		
		//print the builtin endpoints
		printf("SBUS Default Endpoints:\t");
		for (int z = 0; z < com->count_builtins(); z++)
			printf("[%s]\t",com->get_builtin(z)->name);
		printf("\n\n");
		
		printf("%d components:\n", num_cpts);	
		//print the default endpoints
		for(int i = 0; i < num_cpts; i++)
		{
				sn_cpt = reply->tree->extract_item(i);
				sn_metadata = sn_cpt->extract_item("metadata");
				sn_state = sn_cpt->extract_item("state");
				cpt_name = sn_metadata->extract_txt("name");
				instance = sn_state->extract_txt("instance");
				cpt_addr = sn_cpt->extract_txt("address");
				sn_endpoints = sn_state->extract_item("endpoints");
				if(strcmp(cpt_name, instance))
					printf("cpt name: %-15s instance: %-15s addr: %s\n", cpt_name, instance, cpt_addr);
				else
					printf("cpt name: %-15s instance: %-15s addr: %s\n", cpt_name, "-", cpt_addr);
				
				//ignore the builtins --- NB comparison by name.
				for (int j = 0; j < sn_endpoints->count(); j++)
					if (!com->is_builtin(sn_endpoints->extract_item(j)->extract_txt("name")))
						printf("\t[%s]\n",sn_endpoints->extract_item(j)->extract_txt("name") );
				
		}
		delete reply;
	}

	list_serv->unmap();
	dump_serv->unmap();

	delete com;
	return 0;
}
