// democpt.cpp - DMI - 6-4-2007

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include <sys/time.h>

#include "../library/sbus.h"

int log_use_count, stream_use_count;
char *instance_name;

const int count_interval = 1000000;

// Prototypes:
void do_log(scomponent *com, sendpoint *ep);
void do_usage(sendpoint *ep);
void do_submit(sendpoint *ep, sendpoint *news_ep);
void do_news(sendpoint *ep);
void do_delay(sendpoint *ep);
void do_view(sendpoint *ep);
void do_counter(sendpoint *ep, int n);

void usage()
{
	printf("Usage: democpt [instance-name]\n");
	exit(0);
}

void parse_args(int argc, char **argv)
{
	instance_name = NULL;
	
	if(argc > 2)
		usage();
	if(argc == 2)
	{
		if(argv[1][0] == '-')
			usage();
		instance_name = argv[1];
	}
}

int main(int argc, char **argv)
{
	const char *cpt_filename = "demo.cpt";
	scomponent *com;
	sendpoint *ep, *news_ep, *log_ep, *counter_ep;
	int fd;
	multiplex *multi;
	int n = 0;
	struct timeval tv_lastcount, tv_now;
	int micros;
	
	parse_args(argc, argv);
	com = new scomponent("demo", instance_name);
	log_ep = com->add_endpoint("log", EndpointSink, "B732FBF25198");
	com->add_endpoint("usage", EndpointServer, "000000000000", "C50A50E9216F");
	com->add_endpoint("submit", EndpointSink, "D39F55D56AF2");
	news_ep = com->add_endpoint("news", EndpointSource, "D39F55D56AF2");
	com->add_endpoint("delay", EndpointServer, "2632D5B0A26B", "000000000000");
	counter_ep = com->add_endpoint("counter", EndpointSource, "85EC3009D66C");
	com->add_endpoint("viewer", EndpointSink, "85EC3009D66C");
	
	log_use_count = stream_use_count = 0;
	com->start(cpt_filename);
	log_ep->subscribe(NULL, "message ~ 'TIME rolls'", NULL);

	// Get FD's:
	multi = new multiplex();
	for(int i = 0; i < com->count_endpoints(); i++)
	{
		fd = com->get_endpoint(i)->fd;
		multi->add(fd, MULTI_READ);
	}
		
	// Select loop:
	printf("Entering democpt's event loop\n");
	gettimeofday(&tv_lastcount, NULL);
	while(1)
	{
		gettimeofday(&tv_now, NULL);
		micros = (tv_now.tv_sec - tv_lastcount.tv_sec) * 1000000;
		micros += tv_now.tv_usec - tv_lastcount.tv_usec;
		micros = count_interval - micros;
		if(micros <= 0)
			fd = -1;
		else
			fd = multi->wait(micros);
		if(fd < 0)
		{
			do_counter(counter_ep, n++);
			gettimeofday(&tv_lastcount, NULL);
			continue;
		}
		ep = com->fd_to_endpoint(fd);
		if(ep == NULL)
			continue;
		if(!strcmp(ep->name, "log")) do_log(com, ep);
		else if(!strcmp(ep->name, "usage")) do_usage(ep);
		else if(!strcmp(ep->name, "submit")) do_submit(ep, news_ep);
		else if(!strcmp(ep->name, "news")) do_news(ep);
		else if(!strcmp(ep->name, "delay")) do_delay(ep);
		else if(!strcmp(ep->name, "viewer")) do_view(ep);
	}
	
	delete multi;
	delete com;
	return 0;
}

void do_view(sendpoint *ep)
{
	smessage *msg;
	const char *s;
	int n;
	
	msg = ep->rcv();
	s = msg->tree->extract_txt("word");
	n = msg->tree->extract_int("n");
	printf("%s %d\n", s, n);
	delete msg;
}

void do_counter(sendpoint *ep, int n)
{
	snode *sn;
	const char *s;
	
	if(instance_name == NULL)
		s = "default";
	else
		s = instance_name;

	sn = pack(pack(s, "word"), pack(n, "n"), "counter");
	ep->emit(sn, NULL);
	delete sn;
}

void do_log(scomponent *com, sendpoint *ep)
{
	smessage *msg;
	const char *s;
	
	msg = ep->rcv();
	s = msg->tree->extract_txt();
	if(!strcmp(s, "status?"))
	{
		snode *sn;
		
		sn = com->get_status();
		sn->dump();
		delete sn;
	}
	else
	{
		printf("Logging message: %s\n", s);
	}
	delete msg;
	log_use_count++;
}

void do_usage(sendpoint *ep)
{
	smessage *query;
	snode *result;
	
	printf("Component usage() implementation called\n");
	query = ep->rcv();
	result = pack(pack(log_use_count), pack(stream_use_count));
	ep->reply(query, result);
	delete result;
	delete query;
}

void do_submit(sendpoint *ep, sendpoint *news_ep)
{
	smessage *msg;
	
	msg = ep->rcv();
	news_ep->emit(msg->tree, NULL);
	delete msg;
	stream_use_count++;
}

void do_news(sendpoint *ep)
{
	printf("Impossible: source endpoints never return information\n");
	exit(0);
}

void do_delay(sendpoint *ep)
{
	smessage *msg;
	snode *empty = NULL;
	int secs, micros = 0;
	
	msg = ep->rcv();
	secs = msg->tree->extract_int("seconds");
	if(msg->tree->exists("micros"))
		micros = msg->tree->extract_int("micros");
	
	/* Currently this makes the whole component go to sleep; in reality
		we would return from do_delay and set a timer for the reply */
	multiplex *temp = new multiplex();
	temp->wait(secs * 1000000 + micros);
	delete temp;
	
	ep->reply(msg, empty);
	delete msg;
}
