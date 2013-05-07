// extract.cpp - DMI - 22-5-07

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../library/sbus.h"
#include "../wrapper/wrap.h"

const int VERSION = 1;

void check_available(int bytes, int pos, int n)
{
	if(n > 0 && pos + n > bytes)
		error("Unexpected end of data");
}

int read_count(const unsigned char *data, int bytes, int *pos)
{
	int n;
	
	check_available(bytes, *pos, 1);
	n = data[*pos];
	*pos += 1;
	if(n == 254)
	{
		check_available(bytes, *pos, 2);
		n = (data[*pos] << 8) | data[*pos + 1];
		*pos += 2;
	}
	else if(n == 255)
	{
		check_available(bytes, *pos, 4);
		n = (data[*pos] << 24) | (data[*pos + 1] << 16) |
				(data[*pos + 2] << 8) | data[*pos + 3];
		*pos += 4;
	}
	return n;
}

void do_extract(const char *schema_filename, const char *archive_filename)
{
	Schema *schema, *version_schema, *code_schema;
	int blocks;
	HashCode *hc;
	memfile *mf;
	int bytes, pos;
	unsigned char *data;
	int version;
	int length;
	const char *s;
	snode *sn;
	const char *err;
	
	schema = Schema::load(schema_filename, &err);
	if(schema == NULL)
		error("Can't load schema: %s", err);
	version_schema = Schema::create("@int sbus-version", &err);
	if(version_schema == NULL)
		error("Can't create schema: %s", err);
	code_schema = Schema::create("@txt litmus", &err);
	if(code_schema == NULL)
		error("Can't create schema: %s", err);
	
	hc = new HashCode();

	mf = new memfile(archive_filename);
	if(mf->data == NULL)
		error("Couldn't open %s", archive_filename);
	data = (unsigned char *)mf->data;
	bytes = mf->len;
	pos = 0;
	
	check_available(bytes, pos, 5);
	if(data[pos] != 's' || data[pos + 1] != 'B' || data[pos + 2] != 'u'
			|| data[pos + 3] != 'S')
	{
		error("Magic string incorrect");
	}
	version = data[pos + 4];
	if(version != VERSION)
		error("File format version differs");
	pos += 5;
	printf("<sbus-version>%d</sbus-version>\n", version);
	
	check_available(bytes, pos, 6);
	hc->frombinary(data + pos);
	if(!hc->equals(schema->hc))
		error("LITMUS code in archive does not match schema");
	pos += 6;
	
	s = hc->tostring();
	printf("<litmus>%s</litmus>\n", s);
	delete[] s;
	
	blocks = read_count(data, bytes, &pos);
	
	for(int i = 0; i < blocks; i++)
	{
		length = read_count(data, bytes, &pos);
		sn = unmarshall(data + pos, length, schema, &err);
		if(sn == NULL)
			error("Message doesn't conform to schema: %s", err);
		s = sn->toxml(1);
		printf("%s", s);
		delete[] s;
		pos += length;
	}
	if(pos != bytes)
		error("Excess data beyond end of archive");
	
	delete mf;
	delete hc;
	delete code_schema;
	delete version_schema;
	delete schema;
}

void usage()
{
	printf("Usage: extract schema-file archive-file\n");
	exit(0);
}

int main(int argc, char **argv)
{
	if(argc != 3)
		usage();
	do_extract(argv[1], argv[2]);
	return 0;
}
