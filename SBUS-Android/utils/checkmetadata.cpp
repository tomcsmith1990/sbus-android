// checkmetadata.cpp - DMI - 21-5-07

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../library/sbus.h"
#include "../wrapper/wrap.h"

void test_component_metadata(const char *cpt_filename)
{
	const char *metadata_schema = "cpt_metadata.idl";
	Schema *master_schema;
	snode *cpt_metadata;
	const char *err;

	printf("Component metadata test for %s using %s\n", cpt_filename,
			metadata_schema);
	master_schema = Schema::load(metadata_schema, &err);
	if(master_schema == NULL)
		error("Can't load schema: %s", err);
	cpt_metadata = snode::import_file(cpt_filename, &err);
	if(cpt_metadata == NULL)
		error("Error importing metadata from %s:\n%s", cpt_filename, err);
	if(validate(cpt_metadata, master_schema, &err))
		printf("OK, metadata conforms with master schema\n");
	else
		printf("Error: metadata fails to conform with master schema:\n%s\n", err);
	delete cpt_metadata;
	delete master_schema;
}

void usage()
{
	printf("Usage: checkmetadata component-metadata-file ...\n");
	exit(0);
}

int main(int argc, char **argv)
{
	if(argc == 1)
		usage();
	for(int i = 1; i < argc; i++)
	{
		if(argv[i][0] == '-')
			usage();
		else
			test_component_metadata(argv[i]);
	}
	return 0;
}
