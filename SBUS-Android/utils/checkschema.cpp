// checkschema.cpp - DMI - 21-5-07

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../library/sbus.h"
#include "../wrapper/wrap.h"

int verbose = 1;

void test_litmus(const char *schema_filename)
{
	Schema *schema;
	const char *err;
	
	printf("Schema parsing test for %s\n", schema_filename);
	if(verbose > 0) printf("\n");
	schema = Schema::load(schema_filename, &err);
	if(schema == NULL)
		error("Can't load schema: %s", err);
	if(verbose == 2)
		schema->dump_tokens();
	if(verbose != 0)
		schema->dump_tree();
	delete schema;
}

void usage()
{
	printf("Usage: checkschema [-v] [-q] schema-file ...\n");
	printf("       -v = verbose, -q = quiet\n");
	exit(0);
}

int main(int argc, char **argv)
{
	if(argc == 1)
		usage();
	for(int i = 1; i < argc; i++)
	{
		if(!strcmp(argv[i], "-v"))
			verbose = 2;
		else if(!strcmp(argv[i], "-q"))
			verbose = 0;
		else if(argv[i][0] == '-')
			usage();
		else
			test_litmus(argv[i]);
	}
	if(verbose > 0) printf("\n");
	return 0;
}
