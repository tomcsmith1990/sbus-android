// multiplex.cpp - DMI - 12-9-02

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>

#include "error.h"
#include "datatype.h"
#include "dimension.h"
#include "builder.h"
#include "hash.h"
#include "component.h"
#include "multiplex.h"

/* Implementation note:

We either need to make a backup of read_fds and write_fds before each
select() call, or recreate them from another source (e.g. an array or
linked list) because select() overwrites them to indicate which FD's
are ready.

I choose the latter approach (recreating them from an array), because
(a) no standard copy function exists for fd_set's, and (b) after the
call we also need to extract bits using only FD_ISSET, and hence need
a way of enumerating possible fd's in any case. */

multiplex::multiplex(int capacity)
{
	lmode = MULTI_READ;
	rlist = new int[capacity];
	wlist = new int[capacity];
	num_read = num_write = 0;
	this->capacity = capacity;
	pending = 0;
	checked = new sendpointvector();
	label = NULL;
	init_sets();
}

multiplex::~multiplex()
{
	if(label != NULL)
		delete[] label;
	delete[] rlist;
	delete[] wlist;
	delete checked;
}

multi_mode multiplex::last_mode()
{
	return lmode;
}

void multiplex::expand_capacity()
{
	int *big_rlist, *big_wlist;
	
	big_rlist = new int[capacity * 2];
	big_wlist = new int[capacity * 2];
	for(int i = 0; i < capacity; i++)
	{
		big_rlist[i] = rlist[i];
		big_wlist[i] = wlist[i];
	}
	capacity *= 2;
	delete[] rlist;
	delete[] wlist;
	rlist = big_rlist;
	wlist = big_wlist;
}	

void multiplex::add(int fd, multi_mode mode, const char *comment)
{
	if(contains(fd, mode))
		return;
	
	if(label != NULL)
	{
		if(comment == NULL)
		{
			printf("Multiplex %s adding FD %d (%s)\n", label, fd,
					((mode == MULTI_WRITE) ? "write" : "read"));
		}
		else
		{
			printf("Multiplex %s adding FD %d (%s) - %s\n", label, fd,
					((mode == MULTI_WRITE) ? "write" : "read"), comment);
		}
	}
	if(mode == MULTI_WRITE)
	{
		if(num_write == capacity)
			expand_capacity();
		wlist[num_write++] = fd;
	}
	else
	{
		if(num_read == capacity)
			expand_capacity();
		rlist[num_read++] = fd;
	}
}

void multiplex::trace(const char *label)
{
	this->label = sdup(label);
}

void multiplex::add(sendpoint *ep)
{
	int fd = ep->fd;
	
	if(contains(fd, MULTI_READ))
		return;
	
	if(label != NULL)
	{
		printf("Multiplex %s adding FD %d (%s)\n", label, fd, "read");
	}
	if(num_read == capacity)
		expand_capacity();
	rlist[num_read++] = fd;

	checked->add(ep);
}

int multiplex::contains(int fd, multi_mode mode)
{
	int *list;
	int *count;

	if(mode == MULTI_WRITE)
	{ list = wlist; count = &num_write; }
	else
	{ list = rlist; count = &num_read; }
	
	for(int pos = 0; pos < *count; pos++)
		if(list[pos] == fd)
			return 1;
	
	return 0;
}

void multiplex::remove(const char *ep_name, int silent)
{
	for (int j = 0; j < checked->count(); j++)
	{
		if (strcmp(checked->item(j)->name,ep_name)==0)
		{		
			remove(checked->item(j), silent);
			return;
		}
	}
	if (!silent) 
		printf("Endpoint %s not found in multiplex\n",ep_name);
}

void multiplex::remove(sendpoint *ep, int silent)
{
	checked->remove(ep);
	remove(ep->fd, MULTI_READ, silent);
}

void multiplex::remove(int fd, multi_mode mode, int silent)
{
	int *list;
	int *count;
	int pos;

	if(label != NULL)
		printf("Multiplex %s asked to remove FD %d\n", label, fd);
	
	if(mode == MULTI_WRITE)
	{ list = wlist; count = &num_write; }
	else
	{ list = rlist; count = &num_read; }
	
	for(pos = 0; pos < *count; pos++)
		if(list[pos] == fd)
			break;
	
	if(pos == *count)
	{
		if(!silent)
		{
			printf("Warning: multiplex::remove could not find file %s "
					"descriptor %d\n",
					((mode == MULTI_WRITE) ? "MULTI_WRITE" : "MULTI_READ"), fd);
		}
		return; // Not found in list
	}
	
	for(int i = pos; i + 1 < *count; i++)
		list[i] = list[i + 1];
	(*count)--;

	if(pending > 0)
	{
		if(FD_ISSET(fd, &write_fds))
		{
			FD_CLR(fd, &write_fds);
			pending--;
		}
		if(FD_ISSET(fd, &read_fds))
		{
			FD_CLR(fd, &read_fds);
			pending--;
		}
	}	
}

void multiplex::clear()
{
	num_read = num_write = 0;
	pending = 0;
	checked->clear();
}

void multiplex::init_sets()
{
	max_fd = 0;
	FD_ZERO(&read_fds);
	FD_ZERO(&write_fds);
	// printf("Select on read fds: ");
	for(int i = 0; i < num_read; i++)
	{
		// printf("%d  ", rlist[i]);
		FD_SET(rlist[i], &read_fds);
		if(rlist[i] > max_fd)
			max_fd = rlist[i];
	}
	// printf("\nSelect on write fds: ");
	for(int i = 0; i < num_write; i++)
	{
		// printf("%d  ", wlist[i]);
		FD_SET(wlist[i], &write_fds);
		if(wlist[i] > max_fd)
			max_fd = wlist[i];
	}
	// printf("\n");
	// printf("num_read = %d, num_write = %d\n", num_read, num_write);
}

int multiplex::output() // Returns -1 if nothing ready
{
	if(pending > 0)
		pending--;
	for(int i = 0; i < num_write; i++)
	{
		if(FD_ISSET(wlist[i], &write_fds))
		{
			FD_CLR(wlist[i], &write_fds);
			lmode = MULTI_WRITE;
			return wlist[i];
		}
	}
	for(int i = 0; i < num_read; i++)
	{
		if(FD_ISSET(rlist[i], &read_fds))
		{
			FD_CLR(rlist[i], &read_fds);
			lmode = MULTI_READ;
			return rlist[i];
		}
	}
	// Nothing ready after all
	pending = 0;
	return -1; // A fd must have closed since the pending indication
}

int multiplex::fd_waiting()
{
	sendpoint *ep;
	
	for(int i = 0; i < checked->count(); i++)
	{
		ep = checked->item(i);
		if(ep->message_waiting())
			return ep->fd;
	}
	return -1;
}

int multiplex::poll()
{			
	int fd;
	
	if(pending > 0)
	{
		fd = output();
		if(fd != -1) return fd;
	}
	fd = fd_waiting();
	if(fd != -1)
		return fd;
	init_sets();
	tv.tv_sec = tv.tv_usec = 0;
	pending = select(max_fd + 1, &read_fds, &write_fds, NULL, &tv);
	if(pending < 1)
		return -1;
	return output();
}

int multiplex::wait()
{
	int fd;
	
	if(pending > 0)
	{
		fd = output();
		if(fd != -1)
			return fd;
	}
	fd = fd_waiting();
	if(fd != -1)
		return fd;
	while(1) // Loop just in case output() returns -1 at the end
	{
		init_sets();
		pending = select(max_fd + 1, &read_fds, &write_fds, NULL, NULL);
		if(pending == 0)
			error("select() returned timeout, but no timeout set!");
		if(pending < 0)
		{
			if(errno == EINTR)
				continue; // Interrupted by signal; try again
			if(errno == EBADF && label != NULL)
			{
				check_validity();
				error("Multiplex %s includes an invalid FD", label);
			}
			perror("Select error");
			exit(0);
		}
		fd = output();
		if(fd != -1)
			break; // OK fine, we'll return that
		// If not, loop around and try again
	}
	return fd;
}

void multiplex::check_validity()
{
	fd_set test_fds;
	int fd, ret;
	struct timeval tv;
	
	for(int i = 0; i < num_read; i++)
	{
		fd = rlist[i];
		FD_ZERO(&test_fds);
		FD_SET(fd, &test_fds);
		tv.tv_sec = 0;
		tv.tv_usec = 0;
		ret = select(fd + 1, &test_fds, NULL, NULL, &tv);
		if(ret < 0)
			printf("FD %d (read) Bad\n", fd);
		else
			printf("FD %d (read) OK\n", fd);
	}
	for(int i = 0; i < num_write; i++)
	{
		fd = wlist[i];
		FD_ZERO(&test_fds);
		FD_SET(fd, &test_fds);
		tv.tv_sec = 0;
		tv.tv_usec = 0;
		ret = select(fd + 1, &test_fds, NULL, NULL, &tv);
		if(ret < 0)
			printf("FD %d (write) Bad\n", fd);
		else
			printf("FD %d (write) OK\n", fd);
	}
}

int multiplex::wait(int us)
{
	int fd;
	
	if(pending > 0)
	{
		fd = output();
		if(fd != -1)
			return fd;
	}
	fd = fd_waiting();
	if(fd != -1)
		return fd;
	while(1) // Loop just in case output() returns -1 at the end
	{
		init_sets();
		tv.tv_sec = us / 1000000;
		tv.tv_usec = us % 1000000;
		pending = select(max_fd + 1, &read_fds, &write_fds, NULL, &tv);
		if(pending == 0)
			return -1; // Timeout
		if(pending < 0)
		{
			if(errno == EINTR)
				continue; // Interrupted by signal; try again
			if(errno == EBADF && label != NULL)
			{
				check_validity();
				error("Multiplex %s includes an invalid FD", label);
			}
			perror("Select error");
			exit(0);
		}
		fd = output();
		if(fd != -1)
			break; // OK fine, we'll return that
		// If not, loop around and try again (note: may extend timeout, oops!)
	}
	return fd;
}
