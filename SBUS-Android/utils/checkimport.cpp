// checkimport.cpp - DMI - 21-5-07

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../library/sbus.h"
#include "../wrapper/wrap.h"

int verbose = 1;

void test_import(const char *xml_filename)
{
	snode *msg;
	const char *err;
	const char *s;
			
	printf("XML import/export test for %s\n", xml_filename);
	if(verbose > 0) printf("\n");
	msg = snode::import_file(xml_filename, &err);
	if(msg == NULL)
		error("Error importing XML from %s:\n%s", xml_filename, err);
	if(verbose > 0)
		msg->dump();
	s = msg->toxml(1);
	if(verbose == 2)
		printf("%s", s);
	delete s;
	delete msg;
}

void usage()
{
	printf("Usage: checkimport [-q] [-v] xml-file ...\n");
	printf("       -q = quiet, -v = verbose (export as well)\n");
	exit(0);
}

int main(int argc, char **argv)
{
	if(argc == 1)
		usage();
	for(int i = 1; i < argc; i++)
	{
		if(!strcmp(argv[i], "-q"))
			verbose = 0;
		else if(!strcmp(argv[i], "-v"))
			verbose = 2;
		else if(argv[i][0] == '-')
			usage();
		else
			test_import(argv[i]);
	}
	if(verbose > 0) printf("\n");
	return 0;
}
