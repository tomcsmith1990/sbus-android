// checkvalidate.cpp - DMI - 21-5-07

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../library/sbus.h"
#include "../wrapper/wrap.h"

void test_validate(const char *xml_filename, const char *schema_filename)
{
	Schema *schema;
	snode *msg;
	const char *err;
	
	msg = snode::import_file(xml_filename, &err);
	if(msg == NULL)
		error("Error importing XML from %s:\n%s", xml_filename, err);
	
	printf("Validation test for %s using\n   %s:\n", xml_filename,
			schema_filename);
	schema = Schema::load(schema_filename, &err);
	if(schema == NULL)
		error("Can't load schema: %s", err);
	
	if(validate(msg, schema, &err))
		printf("OK, XML conforms with schema\n");
	else
		printf("Error: XML fails to conform with schema: %s\n", err);
	
	delete msg;
	delete schema;
}

void usage()
{
	printf("Usage: checkvalidate schema-file xml-file\n");
	exit(0);
}

int main(int argc, char **argv)
{
	if(argc != 3)
		usage();
	test_validate(argv[2], argv[1]);
	return 0;
}
