// analysecpt.cpp - DMI - 21-5-07

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../library/sbus.h"
#include "../wrapper/wrap.h"

int quiet = 0;

void spaces(int n)
{
	if(n < 1)
		return;
	for(int i = 0; i < n; i++)
		printf(" ");
}

void analyse_component(const char *cpt_filename)
{
	const char *metadata_schema = "cpt_metadata.idl";
	Schema *master_schema;
	snode *cpt_metadata, *sn, *subn;
	const char *name, *type, *msg_idl, *reply_idl;
	Schema *msg_schema, *reply_schema;
	// EndpointType type;
	int num_aps;
	const char *err;
	int chars;
	HashCode *msg_hc, *reply_hc;
	char *hsh;

	master_schema = Schema::load(metadata_schema, &err);
	if(master_schema == NULL)
		error("Can't load master schema: %s", err);
	cpt_metadata = snode::import_file(cpt_filename, &err);
	if(cpt_metadata == NULL)
		error("Error importing metadata from %s:\n%s", cpt_filename, err);
	
	// master_schema->dump_tree();
	// cpt_metadata->dump();
	/*
	char *s = cpt_metadata->toxml(1);
	printf("%s\n", s);
	delete[] s;
	*/
	
	if(!validate(cpt_metadata, master_schema, &err))
		error("Metadata fails to conform with master schema:\n%s\n", err);
		
	sn = cpt_metadata->extract_item("endpoints");
	num_aps = sn->count();
	for(int i = 0; i < num_aps; i++)
	{
		subn = sn->extract_item(i);
		name = subn->extract_txt("name");
		// type = (EndpointType)subn->extract_enum("type");
		type = subn->extract_value("type");
		msg_idl = subn->extract_txt("message");
		reply_idl = subn->extract_txt("response");
		
		msg_schema = Schema::create(msg_idl, &err);
		if(msg_schema == NULL)
			error("Can't create schema: %s", err);
		msg_hc = msg_schema->hc;
		reply_schema = Schema::create(reply_idl, &err);
		if(reply_schema == NULL)
			error("Can't create schema: %s", err);
		reply_hc = reply_schema->hc;
		
		hsh = msg_hc->tostring();
		chars = printf("%s: %s%s", name, type,
				(reply_hc->isapplicable() ? " (message)" : ""));
		spaces(35 - chars);
		printf(" code: %s\n", hsh);
		if(!quiet)
			msg_schema->dump_tree(1);
		delete[] hsh;
		
		if(reply_hc->isapplicable())
		{
			hsh = reply_hc->tostring();
			chars = printf("%s: %s (response)", name, type);
			spaces(35 - chars);
			printf(" code: %s\n", hsh);
			if(!quiet)
				reply_schema->dump_tree(1);
			delete[] hsh;
		}
		
		delete msg_schema;
		delete reply_schema;
	}

	delete cpt_metadata;
	delete master_schema;
}

void usage()
{
	printf("Usage: analysecpt [-q] component-metadata-file ...\n");
	printf("       -q = quiet (don't output full schemas)\n");
	exit(0);
}

int main(int argc, char **argv)
{
	if(argc == 1)
		usage();
	for(int i = 1; i < argc; i++)
	{
		if(!strcmp(argv[i], "-q"))
			quiet = 1;
		else if(argv[i][0] == '-')
			usage();
		else
			analyse_component(argv[i]);
	}
	return 0;
}
