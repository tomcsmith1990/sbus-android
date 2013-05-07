// trafficgen.cpp - DMI - 17-10-2007

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <sys/time.h>

#include "../library/sbus.h"

char *instance_name;

const int delay_ms = 1000;
// const int delay_ms = 50;

const int NUM_COLOURS = 4;
const char *colours[NUM_COLOURS] =
{
	"red", "black", "silver", "blue"
};

// Prototypes:
void do_bus(sendpoint *ep, int speed);
void do_car(sendpoint *ep, int speed);

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

int main(int argc, char **argv)
{
	const char *cpt_filename = "trafficgen.cpt";
	scomponent *com;
	sendpoint *car_ep, *bus_ep;
	struct timeval tv;
	struct timespec ts;
	int speed, thing;
	
	parse_args(argc, argv);
	com = new scomponent("trafficgen", instance_name);
	car_ep = com->add_endpoint("cars", EndpointSource, "727AC14DAC71");
	bus_ep = com->add_endpoint("buses", EndpointSource, "3B6FE4DF2615");
	
	com->start(cpt_filename);

	gettimeofday(&tv, NULL);
	srand((unsigned int)tv.tv_usec);
	
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
	delete sn;
}

void do_car(sendpoint *ep, int speed)
{
	snode *sn;
	const char *colour = colours[rand() % NUM_COLOURS];
	
	printf("Car speed %2d, colour %s\n", speed, colour);
	sn = pack(pack(colour, "colour"), pack(speed, "speed"), "vehicle");
	ep->emit(sn);
	delete sn;
}
