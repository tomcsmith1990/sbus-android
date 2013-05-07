// multitraffic.cpp - DMI - 20-06-2008

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#include <sys/time.h>

#include "../library/sbus.h"

char *instance_name;

const int delay_ms = 1000;

const int NUM_COLOURS = 3;
const char *colours[NUM_COLOURS] =
{
	"red", "black", "silver"
};

// Prototypes:
void do_bus(sendpoint *ep, int speed);
void do_car(sendpoint *ep, int speed);
void do_purple_car(sendpoint *ep, int speed);

void usage()
{
	printf("Usage: trafficgen [instance-name]\n");
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

void *purple_loop(void *arg)
{
	sendpoint *ep;
	struct timespec ts;
	int speed;
	
	ep = (sendpoint *)arg;
	
	while(1)
	{
		ts.tv_sec = delay_ms / 1000;
		ts.tv_nsec = (delay_ms % 1000) * 1000 * 1000;
		nanosleep(&ts, NULL);
		
		speed = rand() % 31;
		do_purple_car(ep, speed);
	}
	
	return NULL;
}

int main(int argc, char **argv)
{
	const char *cpt_filename = "trafficgen.cpt";
	scomponent *com;
	sendpoint *car_ep, *car_alt_ep, *bus_ep;
	struct timeval tv;
	struct timespec ts;
	int speed, thing;
	pthread_t purple_thread;
	int ret;
	
	parse_args(argc, argv);
	com = new scomponent("trafficgen", instance_name);
	car_ep = com->add_endpoint("cars", EndpointSource, "727AC14DAC71");
	bus_ep = com->add_endpoint("buses", EndpointSource, "3B6FE4DF2615");
	
	com->start(cpt_filename);
	car_alt_ep = com->clone(car_ep);
	
	gettimeofday(&tv, NULL);
	srand((unsigned int)tv.tv_usec);
	
	ret = pthread_create(&purple_thread, NULL, &purple_loop, car_alt_ep);
	if(ret != 0)
		error("Can't create a thread");
	
	while(1)
	{
		ts.tv_sec = delay_ms / 1000;
		ts.tv_nsec = (delay_ms % 1000) * 1000 * 1000;
		nanosleep(&ts, NULL);
		
		speed = rand() % 31;
		thing = rand() % 10;
		if(thing < 7)
			do_car(car_ep, speed);
		else
			do_bus(bus_ep, speed);
	}
	
	delete com;
	return 0;
}

void do_bus(sendpoint *ep, int speed)
{
	snode *sn;
	int passengers = rand() % 40;

	printf("Bus speed %2d, passengers %d\n", speed, passengers);	
	sn = pack(pack(passengers, "passengers"), pack(speed, "speed"), "vehicle");
	ep->emit(sn);
}

void do_car(sendpoint *ep, int speed)
{
	snode *sn;
	const char *colour = colours[rand() % NUM_COLOURS];
	
	printf("Car speed %2d, colour %s\n", speed, colour);
	sn = pack(pack(colour, "colour"), pack(speed, "speed"), "vehicle");
	ep->emit(sn);
}

void do_purple_car(sendpoint *ep, int speed)
{
	snode *sn;
	const char *colour = "purple";
	
	printf("Car speed %2d, colour %s\n", speed, colour);
	sn = pack(pack(colour, "colour"), pack(speed, "speed"), "vehicle");
	ep->emit(sn);
}
