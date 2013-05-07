// litmuscode.cpp - DMI - 12-4-2007

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "../library/sbus.h"
#include "../wrapper/wrap.h"
	
typedef unsigned char uchar;

void test(double bits);
void dump_hash(unsigned char *hsh);
void test_hashes();
void test_bits();

char *path;

void usage()
{
	printf("Usage: litmuscode <filename>\n");
	printf("       litmuscode -          (stdin)\n");
	exit(0);
}

void parse_args(int argc, char **argv)
{
	if(argc != 2)
		usage();
	if(argv[1][0] == '-' && argv[1][1] != '\0')
		usage();
	path = argv[1];
}

int main(int argc, char **argv)
{
	Schema *schema;
	const char *err;
	HashCode *hc;
	char *s;
	
	parse_args(argc, argv);
	if(path[0] == '-')
	{
		stdinfile *stdinf = new stdinfile();
		schema = Schema::create(stdinf->data, &err);
		if(schema == NULL)
			error("Can't load schema: %s", err);
		delete stdinf;
	}
	else
	{
		schema = Schema::load(path, &err);
		if(schema == NULL)
			error("Can't load schema from %s:\n%s", path, err);
	}
	hc = schema->hc;
	s = hc->tostring();
	printf("%s\n", s);
	delete s;
	
	// test_hashes();
	// test_bits();
	return 0;
}

void test_hashes()
{
	unsigned char *hsh;
	
	hsh = HashCode::hash("Hello, world!");
	dump_hash(hsh); delete hsh;
	hsh = HashCode::hash("Hello, world, etc");
	dump_hash(hsh); delete hsh;
	hsh = HashCode::hash("Hello");
	dump_hash(hsh); delete hsh;
	hsh = HashCode::hash("123");
	dump_hash(hsh); delete hsh;
	hsh = HashCode::hash("456");
	dump_hash(hsh); delete hsh;
}

void dump_hash(unsigned char *hsh)
{
	// printf("0x");
	for(int i = 0; i < 6; i++)
		printf("%02X", hsh[i]);
	printf("\n");
}

void test_bits()
{
	test(16.0);
	test(32.0);
	test(40.0);
	test(48.0);
	test(56.0);
	test(60.0);
	test(64.0);
}

void test(double bits)
{
	double n = pow(2.0, bits);
	int items = 1, millions = 0;
	double x = 1.0;
	double denom = 1.0 / n;

	printf("Testing %g bit hash:\n", bits);
	while(1)
	{
		// printf("%d items, P(all different hashes) = %g\n", items, x);
		if(x < 0.999999)
			break;
		x *= (n - (double)items - (double)millions * 1000000.0);
		x *= denom;
		items++;
		if(items >= 1000000)
		{
			items -= 1000000;
			millions++;
		}
	}
	if(millions == 0)
		printf("%d items, P(all different hashes) = %g\n", items, x);
	else
		printf("%d%06d items, P(all different hashes) = %g\n", millions, items, x);
	printf("\n");
	return;
}
