// misctest.cpp - DMI - 31-1-2007

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <math.h>
#include <netdb.h>

#include "../library/sbus.h"
#include "../wrapper/wrap.h"

void test_export(snode *msg);

void test_nan()
{
	// float ref = NAN;
	float f = 1.0;
	printf("%g\n", f);
	if(isnan(f)) printf("NAN\n"); else printf("OK\n");
	f = NAN;
	printf("%g\n", f);
	if(isnan(f)) printf("NAN\n"); else printf("OK\n");
	f = -1.2;
	printf("%g\n", f);
	if(isnan(f)) printf("NAN\n"); else printf("OK\n");
	exit(0);
}

void test_float()
{
	double x = 1;
	printf("x = %g %f %e\n", x, x, x);
}

void dump_binary(const unsigned char *data, int len)
{
	printf("%d bytes:\n", len);
	for(int i = 0; i < len; i++)
	{
		printf("%02x", data[i]);
		if(i != len - 1)
			printf(",");
		if(i % 16 == 15)
			printf("\n");
	}
	printf("\n");
}

snode *create_message()
{
	snode *msg;
	int x = 150, y = -200;
	double elev = 6.2;
	const char *s;
	int route = 33;
	
	sdatetime *t = new sdatetime();
	t->set_now();
	s = t->tostring();
	printf("Time now is: %s\n", s);
	delete s;
	
	msg = pack(pack(t), pack(x, "x"), pack(y, "y"), pack(elev),
			pack(route), "bus-position");
	return msg;	
}

snode *send_message(snode *msg, Schema *schema)
{
	unsigned char *data;
	const char *err;
	int len;
	
	data = marshall(msg, schema, &len, &err);
	if(data == NULL)
		error("Marshall failed: %s", err);
	dump_binary(data, len);
	
	snode *reply = unmarshall(data, len, schema, &err);
	if(reply == NULL)
		error("Unmarshall failed: %s", err);
	delete data;	
	return reply;
}

void test_marshalling(const char *litmus_filename)
{
	Schema *schema;
	snode *msg, *reply;
	const char *err;
	
	schema = Schema::load(litmus_filename, &err);
	if(schema == NULL)
		error("Can't load schema: %s", err);
	msg = create_message();
	printf("Created message dump:\n");
	msg->dump();
	test_export(msg);
	reply = send_message(msg, schema);
	delete msg;
	reply->dump();
	delete reply;	
	delete schema;	
}

void test_export(snode *msg)
{
	const char *s;
	
	s = msg->toxml(1);
	printf("XML (pretty-print):\n%s", s);
	delete s;
	s = msg->toxml(0);
	printf("XML (compact):\n%s\n", s);
	delete s;
}

void test_expr(const char *s)
{
	subscription *subs;
	
	printf("Subscription:\n\"%s\"\n", s);
	subs = new subscription(s);
	subs->dump_tokens();
	subs->dump_tree();
	delete subs;
}

void test_expressions()
{
	test_expr("foo/bar > 17 & random?");
	test_expr("vehicle/owner? & vehicle/position.lon < -1.5 &"
		"vehicle/timestamp > '1/6/2006' & vehicle/timestamp < '16/6/2006' &"
		"((vehicle/occupants.items = 2 & vehicle/occupants/#1 = 'fred bloggs') |"
		"(vehicle/description/colour = '*green' & vehicle/speed > 50 &"
		"vehicle/type ~ 'car'))");
}

void test_address()
{
	char *host = new char[100];
	char *domain = new char[100];
	long hostid;
	// struct hostent *he;
	
	if(gethostname(host, 100) < 0)
		printf("Can't get hostname\n");
	if(getdomainname(domain, 100) < 0)
		printf("Can't get domain name\n");
	hostid = gethostid();
	/*
	he = gethostbyaddr((char *)&hostid, 4, AF_INET);
	if(he == NULL)
		printf("Can't get host by addr\n");
	*/
	
	printf("host name = %s\n", host);
	printf("domain name = %s\n", domain);
	printf("host ID = %lx\n", hostid);
	// printf("h_name = %s\n", he->h_name);
}

int main(int argc, char **argv)
{
	printf("FD_SETSIZE = %d\n", FD_SETSIZE);
	// test_expressions();
	
	/*
	test_marshalling("../examples/bus_rpc_schema.idl");
	*/
			
	return 0;
}
