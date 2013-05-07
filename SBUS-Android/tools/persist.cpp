// persist.cpp - DMI - 4-8-2008

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "../library/sbus.h"

#include "rdc.h"

typedef char *charptr;

persistence::persistence()
{
	char *filename;

	procs = NULL;
		
	// Read ~/.sbus/autorun
	sbus_dir = get_sbus_dir();
	filename = new char[strlen(sbus_dir) + 20];	
	sprintf(filename, "%s/autorun", sbus_dir);
	cpts = new linefile(filename, 1);
	if(!cpts->valid())
	{
		warning("Warning: Could not open %s; no persistent components list",
				filename);
	}
	delete[] filename;
}

void persistence::hup(sendpoint *ep, imagevector *live,
		pthread_mutex_t *live_mutex, sendpoint *terminate_ep)
{
	char *filename;
	image *img;
	
	smessage *dummy;
	dummy = ep->rcv();
	delete dummy;
	
	printf("Received hangup instruction: re-checking persistent components "
			"list\n");
	
	// Refresh list of persistent components:
	delete cpts;
	filename = new char[strlen(sbus_dir) + 20];	
	sprintf(filename, "%s/autorun", sbus_dir);
	cpts = new linefile(filename, 1);
	if(!cpts->valid())
	{
		warning("Warning: Could not open %s; no persistent components list",
				filename);
	}
	delete[] filename;
	
	// Scan for any which are no longer persistent, and stop them:
	pthread_mutex_lock(live_mutex);
	for(int i = 0; i < live->count(); i++)
	{
		img = live->item(i);
		if(img->local && img->persistent)
		{
			if(!is_persistent(img->state->extract_txt("cmdline")))
			{
				// Stop it:
				printf("Terminating previously persistent component %s\n",
						img->address);
				img->persistent = 0;
				if(!terminate_ep->map(img->address, "terminate"))
				{
					printf("Failed to terminate %s\n", img->address);
					continue;
				}
				terminate_ep->emit(NULL);
				terminate_ep->unmap();
			}
		}
	}
	pthread_mutex_unlock(live_mutex);
	
	// Start any which are not running:
	start_all();
}

void persistence::start_all()
{
	if(procs != NULL)
		delete procs;
	procs = running_procs();

	for(int i = 0; i < cpts->count(); i++)
	{
		checkquotes(cpts->item(i));
		printf("Persistent cpt %s %s\n", cpts->item(i),
				(is_running(cpts->item(i)) ? "is running" : "is not running"));
		if(!is_running(cpts->item(i)))
			start(cpts->item(i));
	}
}

void persistence::checkquotes(const char *cmd)
{
	while(*cmd != '\0')
	{
		if(*cmd == '"' || *cmd == '\'')
		{
			error("Quotation marks not currently allowed in persistent component"
					" command lines");
		}
		cmd++;
	}
}

svector *persistence::split(const char *s)
{
	svector *v = new svector();
	const char *start;

	while(*s == ' ' || *s == '\t')
		s++;
	start = s;
	while(1)
	{
		if(*s == '\0')
		{
			if(start != s)
				v->add(start, s - start);
			break;
		}
		if(*s == ' ' || *s == '\t')
		{
			v->add(start, s - start);
			while(*s == ' ' || *s == '\t')
				s++;
			start = s;
		}
		else
			s++;
	}
	return v;
}

void persistence::detach()
{
	// Detach from parent
	int fd;
	
	close(0);
	close(1);
	close(2);
	
	fd = open("/dev/null", O_RDWR); // stdin
	dup(fd); // stdout
	dup(fd); // stderr
	
	if(fork() > 0)
		exit(0);
}

void persistence::start(const char *cmd)
{
	int pid;

	// Create new process:
	pid = fork();
	if(pid != 0)
		return;

	detach();
	
	int ret;
	char *filename, **argv;
	svector *v;
	int args;

	v = split(cmd);
	args = v->count();
	filename = v->item(0);
	argv = new charptr[args + 1];
	for(int i = 0; i < args; i++)
		argv[i] = v->item(i);
	argv[args] = NULL;
	
	ret = execvp(filename, argv);
	delete v;
	printf("Warning: could not start persistent component -\n%s\n", cmd);
	printf("%s\n", strerror(errno));
	exit(0);
}

int persistence::is_persistent(const char *cmd)
{
	for(int i = 0; i < cpts->count(); i++)
	{
		if(!strcmp(cpts->item(i), cmd))
			return 1;
	}
	return 0;
}

int persistence::is_running(const char *proc)
{
	for(int i = 0; i < procs->count(); i++)
	{
		if(!strcmp(procs->item(i), proc))
			return 1;
	}
	return 0;
}

persistence::~persistence()
{
	delete[] sbus_dir;
	if(cpts != NULL)
		delete cpts;
	if(procs != NULL)
		delete procs;
}

int persistence::is_an_integer(const char *name)
{
	int pos = 0;
	
	if(name[pos] < '0' || name[pos] > '9')
		return 0;
	pos++;
	
	while(1)
	{
		if(name[pos] == '\0')
			break;
		if(name[pos] < '0' || name[pos] > '9')
			return 0;
		pos++;
		if(pos > 20) // Integer getting too long for buffers
			return 0;
	}
	return 1;
}

svector *persistence::running_procs()
{
	struct dirent *de;
	DIR *dir;
	const char *filename;
	char *pathname, *buf;
	int fd, bytes;
	svector *results = new svector();
	
	// Access /proc/<number>/cmdline
	pathname = new char[80];
	buf = new char[500];
	dir = opendir("/proc");
	if(dir == NULL)
	{
		warning("Cannot open /proc directory; disabling running process check");
		delete[] pathname;
		delete[] buf;
		return results;
	}
	while(1)
	{
		de = readdir(dir);
		if(de == NULL)
			break;
		filename = de->d_name;
		if(!is_an_integer(filename))
			continue;
		sprintf(pathname, "/proc/%s/cmdline", filename);
		fd = open(pathname, O_RDONLY);
		if(fd == -1)
		{
			printf("Warning: could not open %s\n", pathname);
			continue;
		}
		bytes = read(fd, buf, 500);
		close(fd);
		if(buf[bytes - 1] != '\0')
			continue;
		for(int i = 0; i < bytes - 1; i++)
		{
			if(buf[i] == '\0')
				buf[i] = ' ';
		}
		results->add(buf);
	}
	closedir(dir);
	delete[] pathname;
	delete[] buf;
	return results;
}
