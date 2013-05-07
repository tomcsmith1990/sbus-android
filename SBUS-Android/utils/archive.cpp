// archive.cpp - DMI - 21-5-07

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../library/sbus.h"
#include "../wrapper/wrap.h"

const int VERSION = 1;

void do_archive(const char *schema_filename, const char *xml_filename,
	const char *archive_filename)
{
	Schema *schema, *version_schema, *code_schema;
	snode **input;
	int blocks;
	const char *err;
	int version;
	HashCode *hc;
	StringBuf *sb;
	unsigned char *data;
	int length;
	int ret;
	
	schema = Schema::load(schema_filename, &err);
	if(schema == NULL)
		error("Can't load schema: %s", err);
	hc = new HashCode();
	sb = new StringBuf();
	sb->cat("sBuS", 4);
	sb->cat_byte(VERSION);
	
	version_schema = Schema::create("@int sbus-version", &err);
	if(version_schema == NULL)
		error("Can't create schema: %s", err);
	code_schema = Schema::create("@txt litmus", &err);
	if(code_schema == NULL)
		error("Can't create schema: %s", err);
	
	input = snode::import_file_multi(xml_filename, &blocks, &err);
	if(input == NULL)
		error("Error reading XML from %s:\n%s", xml_filename, err);

	printf("Read %d blocks\n", blocks);
	if(blocks < 3)
		error("XML file should have at least three top-level tags");
	
	if(!validate(input[0], version_schema, &err))
		error("First block not valid sbus-version: %s\n", err);
	version = input[0]->extract_int();
	printf("File format version %d\n", version);
	if(version != VERSION)
		error("Versions differ");
	
	if(!validate(input[1], code_schema, &err))
		error("Second block not valid LITMUS code: %s\n", err);
	const char *s = input[1]->extract_txt();
	hc->fromstring(s);
	if(!hc->equals(schema->hc))
		error("Hash code in file doesn't match supplied schema");
	sb->cat(hc);
	sb->cat(blocks - 2); // Won't be true when different types allowed...

	for(int i = 2; i < blocks; i++)
	{
		if(!validate(input[i], schema, &err))
			error("Message %d fails to conform with schema: %s\n", i, err);
		data = marshall(input[i], schema, &length, &err);
		if(data == NULL)
			error("Impossible: validated message could not be marshalled");
		sb->cat(length);
		sb->cat(data, length);
	}

	// Write it out:
	ret = sb->save(archive_filename);
	if(ret < 0)
		printf("Error writing to %s\n", archive_filename);
	delete sb;
	
	for(int i = 0; i < blocks; i++)
		delete input[i];
	delete[] input;
	delete code_schema;
	delete version_schema;
	delete hc;
	delete schema;
}

void usage()
{
	printf("Usage: archive schema-file xml-file archive-file\n");
	printf("       Note: archive-file will be overwritten\n");
	exit(0);
}

int main(int argc, char **argv)
{
	if(argc != 4)
		usage();
	do_archive(argv[1], argv[2], argv[3]);
	return 0;
}
