// spersist.cpp - DMI - 17-11-2008

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <sqlite3.h>

#include "../library/sbus.h"

const char *filename, *address, *endpoint, *instance;

scomponent *com;

sqlite3 *db = NULL;

// Prototypes:
void do_sink(sendpoint *ep);

/*
types table - hash:schema
data table  - date:time:hash:XML
*/

void usage()
{
	printf("Usage: spersist <database-filename> <address> <endpoint>\n");
	exit(0);
}

void parse_args(int argc, char **argv)
{
	char c;
	StringBuf *sb = new StringBuf();
	
	if(argc != 4)
		usage();

	filename = argv[1];
	address = argv[2];
	endpoint = argv[3];
	
	sb = new StringBuf();
	for(int i = 0; i < (int)strlen(address); i++)
	{
		c = address[i];
		if(c >= 'a' && c <= 'z') sb->cat(c);
		if(c >= 'A' && c <= 'Z') sb->cat(c);
		if(c >= '0' && c <= '9') sb->cat(c);
		if(c == '_' || c == '-') sb->cat(c);
	}
	instance = sb->extract();
	delete sb;
}

void initdb()
{
	char **table;
	int rows, cols;
	char *err;
	int ret;
	const char *sql;

	sql = "select name from sqlite_master where type = 'table'";
	ret = sqlite3_get_table(db, sql, &table, &rows, &cols, &err);
	if(ret)
		error("Problem reading sqlite_master table.");
	// printf("%d rows\n", rows);
	sqlite3_free_table(table);
	if(rows == 2)
		return; // OK, database exists
	if(rows != 0)
		error("Unexpected data in sqlite_master.");
	printf("Initialising database.\n");
	sql = "CREATE TABLE types(hash text, schema text)";
	ret = sqlite3_exec(db, sql, NULL, NULL, &err);
	if(ret)
		error("Problem creating table.");
	sql = "CREATE TABLE data(timestamp text, hash text, xml text)";
	ret = sqlite3_exec(db, sql, NULL, NULL, &err);
	if(ret)
		error("Problem creating table.");
}

int main(int argc, char **argv)
{
	const char *cpt_filename = "spersist.cpt";
	sendpoint *ep, *sink_ep;
	int fd;
	char *official_address;
	multiplex *multi;
	int ret;
	
	parse_args(argc, argv);
	
	scomponent::set_log_level(LogErrors | LogWarnings | LogMessages,
			LogErrors | LogWarnings);
	com = new scomponent("spersist", instance);
	sink_ep = com->add_endpoint("sink", EndpointSink, "FFFFFFFFFFFF");
	
	com->start(cpt_filename);
	ret = sqlite3_open(filename, &db);	
	if(ret)
		error("Can't open database %s: %s\n", filename, sqlite3_errmsg(db));
	initdb();

	official_address = sink_ep->map(address, endpoint);
	if(official_address == NULL)
	{
		printf("Failed to map address '%s', endpoint '%s'\n", address, endpoint);
		delete com;
		exit(0);
	}
	delete[] official_address;
	
	// Get FD's:
	multi = new multiplex();
	for(int i = 0; i < com->count_endpoints(); i++)
	{
		fd = com->get_endpoint(i)->fd;
		multi->add(fd, MULTI_READ);
	}
		
	// Select loop:
	while(1)
	{
		fd = multi->wait();
		if(fd < 0)
			continue;
		ep = com->fd_to_endpoint(fd);
		if(ep == NULL)
			continue;
		if(!strcmp(ep->name, "sink"))
			do_sink(ep);
	}
	
	if(db != NULL)
		sqlite3_close(db);

	delete multi;
	delete com;
	return 0;
}

void cache_schema(HashCode *hc, const char *hash)
{
	char **table;
	int rows, cols;
	char *err;
	int ret;
	StringBuf *sb;
	const char *schema;
	
	// Check if hash is in the 'types' table:
	sb = new StringBuf();
	sb->catf("select hash from types where hash = '%s'", hash);
	ret = sqlite3_get_table(db, (const char *)sb->peek(),
			&table, &rows, &cols, &err);
	if(ret)
		error("Problem searching types table for hash code.");
	sqlite3_free_table(table);
	if(rows > 0)
	{
		delete sb;
		return;
	}
	
	// Discover schema:
	schema = com->get_schema(hc);
	
	// Add hash and schema to the 'types' table:
	sb->clear();
	sb->catf("insert into types values('%s', '%s')", hash, schema);
	ret = sqlite3_exec(db, (const char *)sb->peek(), NULL, NULL, &err);
	if(ret)
		error("Problem adding schema to types table.");
	printf("Learned new schema:\n%s\n", schema);
	
	delete[] schema;
	delete sb;
}

void do_sink(sendpoint *ep)
{
	smessage *msg;
	snode *sn;
	char *xml;
	HashCode *hc;
	char *hash;
	time_t tt;
	struct tm *ts;
	StringBuf *sb;
	int ret;
	char *err;

	tt = time(NULL);
	ts = gmtime(&tt);	
	
	sb = new StringBuf();
	
	msg = ep->rcv();
	sn = msg->tree;
	xml = sn->toxml(1);
	hc = msg->hc;
	hash = hc->tostring();
	cache_schema(hc, hash);
	
	sb->cat("insert into data values(");
	sb->catf("'%04d-%02d-%02d %02d:%02d:%02d', ",
			ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday,
			ts->tm_hour, ts->tm_min, ts->tm_sec);
	sb->catf("'%s', '%s')", hash, xml);
	// printf("%s\n", (const char *)sb->peek());
	ret = sqlite3_exec(db, (const char *)sb->peek(), NULL, NULL, &err);
	printf("."); fflush(stdout);
	
	delete sb;
	delete[] hash;
	delete[] xml;
	delete msg;
}
