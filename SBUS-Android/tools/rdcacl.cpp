// rdc.cpp - DMI - 22-6-2007

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>


#include "../library/sbus.h"
#include "../wrapper/wrap.h"

#include "rdcacl.h"

int port = -1, bg = 0;

const int check_ms = 100;
// const int check_ms = 50;

const int max_pool_size = 10;

const char *permission_file = NULL;

svector *joins;

// Prototypes:
void copy_time(struct timeval *from, struct timeval *to);

void usage()
{
	printf("Usage:   rdc [ <option> ... ]\n");
	printf("Options: -bg       = run in background\n");
	printf("         -p <port> = port number\n");
	printf("         -a        = any port\n");
	printf("         -j <addr> = join other RDC\n");
	printf("         -f <file> = try to join multiple RDCs\n");
	printf("         -x <file> = load ACL privileges from file\n");
	exit(0);
}

void parse_args(int argc, char **argv)
{
	joins = new svector();
	for(int i = 1; i < argc; i++)
	{
		if(!strcmp(argv[i], "-bg"))
			bg = 1;
		else if(!strcmp(argv[i], "-p"))
		{
			i++;
			if(i >= argc)
				usage();
			if(port != -1)
				usage();
			port = atoi(argv[i]);
			if(port < 1 || port > 65535)
				usage();
		}
		else if(!strcmp(argv[i], "-a"))
		{
			if(port != -1)
				usage();
			port = 0;
		}
		else if(!strcmp(argv[i], "-j"))
		{
			i++;
			if(i >= argc)
				usage();
			joins->add(argv[i]);
		}
		else if(!strcmp(argv[i], "-f"))
		{
			linefile *lf;
			
			i++;
			if(i >= argc)
				usage();
			lf = new linefile(argv[i], 1);
			for(int j = 0; j < lf->count(); j++)
				joins->add(lf->getline(j));
			delete lf;
		}
		else if(!strcmp(argv[i], "-x"))
		{
			i++;
			if(i >= argc)
				usage();
			permission_file = sdup(argv[i]);
		}
		else
			usage();
	}
	if(port == -1)
		port = default_rdc_port;
}

int main(int argc, char **argv)
{
	rdc *obj;
	
	parse_args(argc, argv);
	if(bg)
	{
		int fd;
		
		if(fork() > 0) exit(0); // Detach
		fd = open("/dev/null", O_RDWR);
		if(fd < 0) error("Cannot open /dev/null");
		if(close(STDIN_FILENO) < 0) error("Cannot close stdin");
		if(close(STDOUT_FILENO) < 0) error("Cannot close stdout");
		if(close(STDERR_FILENO) < 0) error("Cannot close stderr");
		dup2(fd, STDIN_FILENO);
		dup2(fd, STDOUT_FILENO);
		dup2(fd, STDERR_FILENO);
	}
	obj = new rdc();
	obj->mainloop();

	delete obj;
	delete joins;
	return 0;
}

rdc::rdc()
{
	persist = NULL;
	scomponent::set_log_level(LogErrors | LogWarnings | LogMessages,
			LogErrors | LogWarnings);
	com = new scomponent("rdc");

	register_ep = com->add_endpoint("register", EndpointSink, "B3572388E4A4");
	lost_ep = com->add_endpoint("lost", EndpointSink, "B3572388E4A4");
	com->add_endpoint("hup", EndpointSink, "000000000000");
	com->add_endpoint("lookup_cpt", EndpointServer, "18D70E4219C8",
			"F96D2B7A73C1");
	com->add_endpoint("list", EndpointServer, "000000000000", "46920F3551F9");
	com->add_endpoint("cached_metadata", EndpointServer, "872A0BD357A6",
			"6306677BFE43");
	com->add_endpoint("cached_status", EndpointServer, "872A0BD357A6",
			"253BAC1C33C7");
	com->add_endpoint("dump", EndpointServer, "000000000000", "534073C1E375");
	events_ep = com->add_endpoint("events", EndpointSource, "B3572388E4A4");
	metadata_ep = com->add_endpoint("get_metadata", EndpointClient,
			"000000000000", "6306677BFE43");
	status_ep = com->add_endpoint("get_status", EndpointClient,
			"000000000000", "253BAC1C33C7");
	dump_client_ep = com->add_endpoint("dump_client", EndpointClient,
			"000000000000", "534073C1E375");
	terminate_ep = com->add_endpoint("terminate", EndpointSource,
			"000000000000");
	com->add_endpoint("remote_start", EndpointSink, "8F720B145518");
//	acl_ep = com->add_endpoint("set_acl", EndpointSink, "AB248E248B36"); ---real version, with no 'checkperms hack'
	acl_ep = com->add_endpoint("set_acl", EndpointSink, "6AF2ED96750B");
	
	live = new imagevector();
	registering = new imagevector();
	//permissions = new rdcpermissionstore();

	metadata_epv = new sendpointvector();
	status_epv = new sendpointvector();
	pool_busy = new intvector();
	

	pthread_mutex_init(&live_mutex, NULL);
	pthread_mutex_init(&pool_mutex, NULL);
	pthread_mutex_init(&events_mutex, NULL);
	pthread_mutex_init(&registering_mutex, NULL);
	
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);	
	
	gettimeofday(&tv_start, NULL);
}

rdc::~rdc()
{
	if(persist != NULL)
		delete persist;
	delete com;
	for(int i = 0; i < live->count(); i++)
		delete live->item(i);
	delete live;
}

// Only call this from the master thread, since it may call clone():
int rdc::first_free_pool()
{
	int pool_id;
	
	pthread_mutex_lock(&pool_mutex);	
	for(int i = 0; i < pool_busy->count(); i++)
	{
		if(pool_busy->item(i) == 0)
		{
			pool_busy->set(i, 1);
			pthread_mutex_unlock(&pool_mutex);
			// printf("allocate pool %d\n", i);
			return i;
		}
	}
	if(pool_busy->count() == max_pool_size)
	{
		printf("Serious warning: maximum thread pool size reached; "
				"registrations not being processed\n");
		return -1;
	}
	pool_busy->add(1);
	metadata_epv->add(com->clone(metadata_ep));
	status_epv->add(com->clone(status_ep));
	pool_id = pool_busy->count() - 1;
	pthread_mutex_unlock(&pool_mutex);
	// printf("allocate pool %d\n", pool_id);
	return pool_id;
}

void release_pool(int pool_id, pthread_mutex_t *pool_mutex_ptr,
		intvector *pool_busy)
{
	// printf("release_pool %d\n", pool_id);
	pthread_mutex_lock(pool_mutex_ptr);
	pool_busy->set(pool_id, 0);
	pthread_mutex_unlock(pool_mutex_ptr);
}

void rdc::checkbuiltins()
{
	sbuiltin *bi;
	int num;
	
	num = com->count_builtins();
	printf("%d builtins:\n", num);
	for(int i = 0; i < num; i++)
	{
		bi = com->get_builtin(i);
		printf("%d. %s\n", i, bi->name);
	}
}

int is_whitespace(const char *s)
{
	int len = strlen(s);
	
	for(int i = 0; i < len; i++)
		if(s[i] != ' ' && s[i] != '\t' && s[i] != '\n')
			return 0;
	return 1;
}

void rdc::join()
{
	const char *addr, *fulladdr;
	smessage *msg;
	snode *sn, *subn;	
	int num_cpts;
	const char *cptaddr;
	image *img;
	int known;
	const char *our_address;
	char *remote_address;
	
	our_address = com->get_address();
	printf("Our address is %s\n", our_address);
	for(int i = 0; i < joins->count(); i++)
	{
		addr = joins->item(i);
		if(is_whitespace(addr))
			continue;
		// Add default port, if not specified by addr:
		fulladdr = ipaddress::check_add_port(addr, default_rdc_port);
		
		remote_address = dump_client_ep->map(fulladdr, "dump");
		if(remote_address == NULL)
		{
			printf("RDC %s not running; skipping it\n", addr);
			continue;
		}
		
		// Check we haven't bound to ourselves:
		if(!strcmp(our_address, remote_address))
		{
			printf("Attempt to connect to %s bound to self; skipping it\n",
					addr);
			dump_client_ep->unmap();
			continue;
		}
		else
			printf("Joined to RDC at %s\n", remote_address);
		delete[] remote_address;
		
		msg = dump_client_ep->rpc(NULL);
		if(msg == NULL)
		{
			printf("Warning: could not dump memory from joined RDC %s\n", addr);
			continue;
		}
		sn = msg->tree;
		num_cpts = sn->count();
		for(int j = 0; j < num_cpts; j++)
		{
			// Add component to memory, if not already known:
			subn = sn->extract_item(j);
			cptaddr = subn->extract_txt("address");
			
			// Check not already known:
			known = 0;
			for(int k = 0; k < live->count(); k++)
			{
				img = live->item(k);
				if(!strcmp(img->address, cptaddr))
				{
					known = 1;
					break;
				}
			}
			if(known)
				continue;
	
			img = new image();
			img->address = sdup(cptaddr);
			img->metadata = new snode(subn->extract_item("metadata"));
			img->state = new snode(subn->extract_item("state"));			
			gettimeofday(&(img->tv_ping), NULL);
			img->local = is_local(img->address);
			if(img->local)
			{
				img->persistent =
					persist->is_persistent(img->state->extract_txt("cmdline"));
			}
			else
				img->persistent = 0;
			img->init_hashes();
			live->add(img);



			printf("Registered component %s at %s\n",
					img->metadata->extract_txt("name"), img->address);
		}
		delete msg;
		dump_client_ep->unmap();
		
		// Map register to events, and vice versa:
		register_ep->map(fulladdr, "events");
		events_ep->map(fulladdr, "register");
		delete[] fulladdr;
	}
}

int rdc::is_local(const char *address)
{
	const char *ptr;
	
	ptr = address;
	while(*ptr != ':' && *ptr != '\0')
		ptr++;
	if(*ptr == '\0' || ptr == address)
		error("Invalid address passed to is_local()");
	if(!strncmp(address, local_address, ptr - address))
		return 1;
	return 0;
}

void rdc::read_local_address()
{
	const char *addr, *ptr;
	
	addr = com->get_address();
	ptr = addr;
	while(*ptr != ':' && *ptr != '\0')
		ptr++;
	if(*ptr == '\0' || ptr == addr)
		error("Invalid address returned by scomponent::get_address()");
	local_address = new char[ptr - addr + 1];
	strncpy(local_address, addr, ptr - addr);
	local_address[ptr - addr] = '\0';
}

void rdc::mainloop()
{
	const char *cpt_filename = "rdcacl.cpt";
	int fd;
	multiplex *multi;
	sendpoint *ep;
	struct timeval tv_lastcheck, tv_now;
	int micros;

	if(port == 0)
		com->start(cpt_filename, -1, false); // Use any port
	else	
		com->start(cpt_filename, port, false);
	
	builtin_names = com->get_builtin_names();

	//load defaults
	printf("\n");
	load_permissions(path_lookup("rdc_default.priv"));
	//need to load ACL polcies AFTER we connect to the wrapper
	if (permission_file != NULL)
		load_permissions(permission_file);

	read_local_address();
	persist = new persistence();
	join();

	// Get FD's:
	multi = new multiplex();
	for(int i = 0; i < com->count_endpoints(); i++)
	{
		fd = com->get_endpoint(i)->fd;
		multi->add(fd, MULTI_READ);
	}
		
	persist->start_all(); // Start persistent components
	
	// Select loop:
	gettimeofday(&tv_lastcheck, NULL);
	while(1)
	{
		gettimeofday(&tv_now, NULL);
		micros = (tv_now.tv_sec - tv_lastcheck.tv_sec) * 1000000;
		micros += tv_now.tv_usec - tv_lastcheck.tv_usec;
		micros = check_ms * 1000 - micros;
		if(micros <= 0)
			fd = -1;
		else
			fd = multi->wait(micros);
		if(fd < 0)
		{
			// Timeout: do checkalive
			schedule_check();
			gettimeofday(&tv_lastcheck, NULL);
			continue;
		}
		ep = com->fd_to_endpoint(fd);
		if(ep == NULL)
			continue;
		if(!strcmp(ep->name, "register")) registercpt();
		else if(!strcmp(ep->name, "lost")) lost();
		else if(!strcmp(ep->name, "hup"))
			persist->hup(ep, live, &live_mutex, terminate_ep);
		else if(!strcmp(ep->name, "lookup_cpt")) lookup(ep);
		else if(!strcmp(ep->name, "list")) list(ep);
		else if(!strcmp(ep->name, "dump")) dump(ep);
		else if(!strcmp(ep->name, "remote_start")) remotestart(ep);
		else if(!strcmp(ep->name, "set_acl")) change_permissions(ep);
	}
	
	delete multi;
}

void rdc::remotestart(sendpoint *ep)
{
	smessage *msg;
	const char *cmdline;

	msg = ep->rcv();
	cmdline = msg->tree->extract_txt();
	printf("Remote starting <%s>\n", cmdline);
	persist->start(cmdline);
	delete msg;
}

void rdc::schedule_check()
{
	pthread_t thread;
	int pool_id, ret;
	threadarg *ta;

	pool_id = first_free_pool();
	if(pool_id == -1)
		return; // Omit checkalive

	ta = new threadarg;
	ta->obj = this;
	ta->pool_id = pool_id;
	ret = pthread_create(&thread, &attr, &checkalive, ta);
	if(ret != 0) error("Can't create a thread");
}

void rdc::lost()
{
	smessage *msg;
	const char *addr;
	image *img;
	
	msg = lost_ep->rcv();
	addr = msg->tree->extract_txt("address");
	
	int found = 0;
	pthread_mutex_lock(&live_mutex);
	for(int i = 0; i < live->count(); i++)
	{
		img = live->item(i);
		if(!strcmp(img->address, addr))
		{
			if(img->lost)
			{
				pthread_mutex_unlock(&live_mutex);
				return;
			}
			found = 1;
			break;
		}
	}
	if(!found)
	{
		pthread_mutex_unlock(&live_mutex);
		return;
	}
	printf("Received report that we have lost contact with %s;\n"
			"   scheduling it for immediate liveness check\n", addr);
	img->lost = 1;
	copy_time(&tv_start, &(img->tv_ping)); // Mark as need to check urgently
	pthread_mutex_unlock(&live_mutex);
	schedule_check(); // Extra non-periodic check
	
	delete msg;
}

void rdc::registercpt()
{
	smessage *msg;
	const char *addr;
	image *img, *tmpimg;
	int arrived;

	msg = register_ep->rcv();

	addr = msg->tree->extract_txt("address");
	arrived = msg->tree->extract_flg("arrived");

	printf("REGISTERING cpt %s\n",addr);

	if(strcmp(msg->source_ep, "events"))
	{
		// OK, event isn't from another RDC, so we need to broadcast it:
		pthread_mutex_lock(&events_mutex);
		events_ep->emit(msg->tree);
		pthread_mutex_unlock(&events_mutex);
	}


	if(!arrived)
	{
		deregister(addr);
		delete msg;
		return;
	}
	
	// Check not already registered:
	pthread_mutex_lock(&live_mutex);
	for(int i = 0; i < live->count(); i++)
	{
		img = live->item(i);
		if(!strcmp(img->address, addr))
		{
			pthread_mutex_unlock(&live_mutex);
			printf("Warning: asked to register already-registered component"
					" %s.\n", img->address);
			delete msg;
			return;
		}
	}	
	pthread_mutex_unlock(&live_mutex);
	

	//keep track that this component is being registered
	tmpimg = new image();
	tmpimg->address = sdup(addr);
	tmpimg->cpt_name = sdup(msg->source_cpt);
	tmpimg->ins_name = sdup(msg->source_inst);
	pthread_mutex_lock(&registering_mutex);
	registering->add(tmpimg);
	pthread_mutex_unlock(&registering_mutex);
	printf("Tracking this instance %s:%s at %s in the registering vector\n",tmpimg->cpt_name, tmpimg->ins_name, tmpimg->address);


	img = new image();
	img->address = sdup(addr);
	delete msg;
	
	img->local = is_local(img->address);
	img->persistent = 0; // Until we know better, from the status structure
	
	// Call component back to get status and metadata:

	pthread_t thread;
	int pool_id, ret;
	threadarg *ta;

	pool_id = first_free_pool();
	if(pool_id == -1)
	{
		// Drop registration on the floor
		delete img;
		return;
	}
	
	ta = new threadarg;
	ta->obj = this;
	ta->pool_id = pool_id;
	ta->img = img;

	ret = pthread_create(&thread, &attr, &registerback, ta);
	if(ret != 0) error("Can't create a thread");
}	

void *registerback(void *arg)
{
	smessage *info;
	image *img;
	const char *addr;
	
	threadarg *ta;
	rdc *obj;
	persistence *persist;
	int pool_id;
	sendpoint *status_ep, *metadata_ep;
	pthread_mutex_t *live_mutex_ptr, *registering_mutex_pointer;
	imagevector *live;

	ta = (threadarg *)arg;
	obj = ta->obj;
	persist = obj->persist;
	pool_id = ta->pool_id;
	img = ta->img;
	addr = img->address;
	debug("registerback() using pool slot %d\n", ta->pool_id);
	status_ep = obj->status_epv->item(pool_id);
	metadata_ep = obj->metadata_epv->item(pool_id);
	live_mutex_ptr = &(obj->live_mutex);
	registering_mutex_pointer = &(obj->registering_mutex);
	live = obj->live;
	
	if(!metadata_ep->map(addr, "get_metadata", 0))
	{
		printf("Warning: RDC could not connect back to registering "
				"component at '%s'.\n", addr);
		release_pool(pool_id, &(obj->pool_mutex), obj->pool_busy);
		delete img;
		delete ta;
		return NULL;
	}
	info = metadata_ep->rpc(NULL);
	if(info == NULL)
	{
		printf("Warning: RDC could not fetch metadata for registering "
				"component %s\n", addr);
		metadata_ep->unmap();
		release_pool(pool_id, &(obj->pool_mutex), obj->pool_busy);
		delete img;
		delete ta;
		return NULL;
	}
	img->metadata = info->tree;
	info->tree = NULL;
	delete info;
	metadata_ep->unmap();
	
	if(!status_ep->map(addr, "get_status", 0))
	{
		printf("Warning: RDC could not connect back to registering component.\n");
		release_pool(pool_id, &(obj->pool_mutex), obj->pool_busy);
		delete img;
		delete ta;
		return NULL;
	}
	info = status_ep->rpc(NULL);
	if(info == NULL)
	{
		printf("Warning: RDC could not fetch status for registering "
				"component %s\n", addr);
		status_ep->unmap();
		release_pool(pool_id, &(obj->pool_mutex), obj->pool_busy);
		delete img;
		delete ta;
		return NULL;
	}
	img->state = info->tree;
	info->tree = NULL;
	delete info;
	status_ep->unmap();

	gettimeofday(&(img->tv_ping), NULL);
	
	img->init_hashes();


	img->cpt_name = strdup(img->metadata->extract_txt("name"));
	img->ins_name = strdup(img->state->extract_txt("instance"));
	if(!strcmp(img->cpt_name, img->ins_name))
		printf("Registered component %s at %s\n", img->cpt_name, img->address);
	else
	{
		printf("Registered component %s, instance %s, at %s\n",
				img->cpt_name, img->ins_name, img->address);
	}
	printf("Command line was \"%s\"\n", img->state->extract_txt("cmdline"));
	if(img->local && persist->is_persistent(img->state->extract_txt("cmdline")))
		img->persistent = 1;
	printf("local = %d, persistent = %d\n", img->local, img->persistent);

	//process any remaining
	//TODO: manage concurrency!!
	pthread_mutex_lock(live_mutex_ptr);
	pthread_mutex_lock(registering_mutex_pointer);
	
	// start the checkalive mutex on the image.
	pthread_mutex_init(&(img->checkalive_mutex), NULL);
		
	live->add(img);

	image *tmp;
	for (int i = 0; i < obj->registering->count(); i++)
	{
		tmp = obj->registering->item(i);
		if (tmp->similar(img->cpt_name,img->ins_name,img->address))
		{
			printf("Now processing %d buffered access registrations\n",tmp->buffered_policies->count());
			for (int j = 0; j < tmp->buffered_policies->count(); j++)
				obj->process_permission_change(tmp->buffered_policies->item(j));
				//printf("number %d completed\n",j+1);
				//TODO: Bug about ordering etc, but seems OK in most cases.
		}
	}
	pthread_mutex_unlock(registering_mutex_pointer);
	pthread_mutex_unlock(live_mutex_ptr);

	if (tmp!=NULL){
		obj->registering->remove(tmp);
		delete tmp;
	}


	release_pool(pool_id, &(obj->pool_mutex), obj->pool_busy);
	delete ta;
	return NULL;
}

void rdc::deregister(const char *addr)
{
	image *img;
	int pos;
	
	pthread_mutex_lock(&live_mutex);
	for(pos = 0; pos < live->count(); pos++)
	{
		img = live->item(pos);
		if(!strcmp(img->address, addr))
			break;
	}
	if(pos == live->count())
	{
		pthread_mutex_unlock(&live_mutex);
		return;
	}
	live->del(pos);
	pthread_mutex_unlock(&live_mutex);
	printf("Deregistered component %s at %s\n",
			img->metadata->extract_txt("name"), img->address);
	if(img->local && img->persistent)
	{
		const char *cmd = img->state->extract_txt("cmdline");
		printf("Attempting to restart persistent component:\n");
		printf("   %s\n", cmd);
		persist->start(sdup(cmd));
		// Memory leak here just in case fork() doesn't copy it right
	}
	delete img;
}

void copy_time(struct timeval *from, struct timeval *to)
{
	to->tv_sec = from->tv_sec;
	to->tv_usec = from->tv_usec;
}

int compare_time(struct timeval *tv1, struct timeval *tv2)
{
	if(tv1->tv_sec < tv2->tv_sec)
		return -1;
	else if(tv1->tv_sec > tv2->tv_sec)
		return 1;
	else if(tv1->tv_usec < tv2->tv_usec)
		return -1;
	else if(tv1->tv_usec > tv2->tv_usec)
		return 1;
	
	return 0;
}

// checkalive() could block in status_ep rpc, so separate threads run it:
void *checkalive(void *arg)
{
	image *img, *imgx;
	struct timeval tv_oldest;
	smessage *info;
	threadarg *ta;
	rdc *obj;
	int pool_id;
	sendpoint *status_ep, *metadata_ep;
	pthread_mutex_t *live_mutex_ptr;
	pthread_mutex_t *checkalive_mutex_ptr;
	imagevector *live;
	const char *location;

	ta = (threadarg *)arg;
	obj = ta->obj;
	pool_id = ta->pool_id;
	status_ep = obj->status_epv->item(pool_id);
	metadata_ep = obj->metadata_epv->item(pool_id);
	live_mutex_ptr = &(obj->live_mutex);
	live = obj->live;
	
	pthread_mutex_lock(live_mutex_ptr);
	if(live->count() == 0)
	{
		pthread_mutex_unlock(live_mutex_ptr);
		release_pool(pool_id, &(obj->pool_mutex), obj->pool_busy);
		delete ta;
		return NULL;
	}
		
	// Find the image least recently checked:
	img = live->item(0);
	copy_time(&(img->tv_ping), &tv_oldest);
	for(int i = 0; i < live->count(); i++)
	{
		imgx = live->item(i);
		if(compare_time(&(imgx->tv_ping), &tv_oldest) < 0)
		{
			img = imgx;
			copy_time(&(imgx->tv_ping), &tv_oldest);
		}
	}
	pthread_mutex_unlock(live_mutex_ptr);
	
	checkalive_mutex_ptr = &(img->checkalive_mutex);

	if (pthread_mutex_trylock(checkalive_mutex_ptr) != 0)
	{
		/*
		 * pthread_mutex_trylock will acquire the lock if it is free, and return 0.
		 * It returns immediately.
		 * The only reason it wouldn't be able to acquire the lock is if we're already checking liveness on this image.
		 * If we're currently checking it, don't need to check again immediately, so we can release this thread and return.
		 */
		release_pool(pool_id, &(obj->pool_mutex), obj->pool_busy);
		return NULL;
	}
	
	const char *cpt_name = img->metadata->extract_txt("name");
	const char *cpt_instance = img->state->extract_txt("instance");
	if(cpt_instance != NULL && strcmp(cpt_name, cpt_instance) != 0)
	{
		log("Checking component %s(%s) %s still live\n",
				cpt_name, cpt_instance, img->address);
	}
	else
	{
		log("Checking component %s at %s still live\n",
				cpt_name, img->address);
	}

	// Ping it by asking for new status:
	location = status_ep->map(img->address, "get_status", 0);
	if(location == NULL)
	{
		printf("Ping indicates component %s at %s vanished\n"
				"   without deregistering; removing it from list\n",
				img->metadata->extract_txt("name"), img->address);
		pthread_mutex_lock(live_mutex_ptr);
		live->remove(img);
		pthread_mutex_unlock(live_mutex_ptr);
		pthread_mutex_unlock(checkalive_mutex_ptr);
		release_pool(pool_id, &(obj->pool_mutex), obj->pool_busy);
		if(img->local && img->persistent)
		{
			const char *cmd = img->state->extract_txt("cmdline");
			printf("Attempting to restart persistent component:\n");
			printf("   %s\n", cmd);
			obj->persist->start(sdup(cmd));
			// Memory leak here just in case fork() doesn't copy it right
		}
		delete img;
		delete ta;
		return NULL;
	}
	sfree(location);
	info = status_ep->rpc(NULL);
	if(info == NULL)
	{
		printf("Component disconnected during ping (unusual); "
				"removing it from list\n");
		pthread_mutex_lock(live_mutex_ptr);
		live->remove(img);
		pthread_mutex_unlock(live_mutex_ptr);
		pthread_mutex_unlock(checkalive_mutex_ptr);
		release_pool(pool_id, &(obj->pool_mutex), obj->pool_busy);
		if(img->local && img->persistent)
		{
			const char *cmd = img->state->extract_txt("cmdline");
			printf("Attempting to restart persistent component:\n");
			printf("   %s\n", cmd);
			obj->persist->start(sdup(cmd));
			// Memory leak here just in case fork() doesn't copy it right
		}
		delete img;
		delete ta;
		return NULL;
	}
	// Update status:
	delete img->state;
	img->state = info->tree;
	info->tree = NULL;
	delete info;
	status_ep->unmap();
	
	// Update its ping time:
	gettimeofday(&(img->tv_ping), NULL);

	// Ask for new metadata:
	location = metadata_ep->map(img->address, "get_metadata", 0);
	if(location == NULL)
	{
		printf("Ping indicates component %s at %s vanished\n"
				"   without deregistering; removing it from list\n",
				img->metadata->extract_txt("name"), img->address);
		pthread_mutex_lock(live_mutex_ptr);
		live->remove(img);
		pthread_mutex_unlock(live_mutex_ptr);
		pthread_mutex_unlock(checkalive_mutex_ptr);
		release_pool(pool_id, &(obj->pool_mutex), obj->pool_busy);
		if(img->local && img->persistent)
		{
			const char *cmd = img->state->extract_txt("cmdline");
			printf("Attempting to restart persistent component:\n");
			printf("   %s\n", cmd);
			obj->persist->start(sdup(cmd));
			// Memory leak here just in case fork() doesn't copy it right
		}
		delete img;
		delete ta;
		return NULL;
	}
	sfree(location);
	info = metadata_ep->rpc(NULL);
	if(info == NULL)
	{
		printf("Component disconnected during get_metadata (unusual); "
				"removing it from list\n");
		pthread_mutex_lock(live_mutex_ptr);
		live->remove(img);
		pthread_mutex_unlock(live_mutex_ptr);
		pthread_mutex_unlock(checkalive_mutex_ptr);
		release_pool(pool_id, &(obj->pool_mutex), obj->pool_busy);
		if(img->local && img->persistent)
		{
			const char *cmd = img->state->extract_txt("cmdline");
			printf("Attempting to restart persistent component:\n");
			printf("   %s\n", cmd);
			obj->persist->start(sdup(cmd));
			// Memory leak here just in case fork() doesn't copy it right
		}
		delete img;
		delete ta;
		return NULL;
	}
	// Update metadata:
	delete img->metadata;
	img->metadata = info->tree;
	info->tree = NULL;
	delete info;
	metadata_ep->unmap();
	pthread_mutex_unlock(checkalive_mutex_ptr);
	release_pool(pool_id, &(obj->pool_mutex), obj->pool_busy);
	delete ta;
	return NULL;
}


void rdc::load_permissions(const char *filename)
{
	int num_entries;
	const char *err;
	snode **input;
	Schema *schema;

	printf("Loading permissions from file %s\n",filename);
	input = snode::import_file_multi(filename, &num_entries, &err);
	if(input == NULL)
		error("Error reading XML from %s:\n%s\n", filename, err);

	//convert to appropriate schema
	//TODO: Must be a better way than this!
	schema = Schema::create(com->get_schema(acl_ep->msg_hc), &err);
	if (schema==NULL)
			error("Error reading schema %s\n", err);

	for (int i = 0; i < num_entries; i++)
	{

		if(!validate(input[i], schema, &err))
			error("Metadata fails to conform with master schema:\n%s\n", err);
		process_permission_change(input[i]);
	}
}

void rdc::change_permissions(sendpoint *ep){
	smessage *perms;

	perms = ep->rcv();
	int process = false;



	//printf("got a change request %s\n",perms->tree->toxml(1));
	//Components can only update their OWN acl, unless they have special privileges...
	//OR -- SUPEUSER
	if (
			//check whether the issuing and target components differ...
			strcmp(perms->source_cpt,perms->tree->extract_txt("target_cpt")) ||
			strcmp(perms->source_inst,perms->tree->extract_txt("target_inst")) )
	{
		printf("A rule was issued by a component/inst for some 3rd party one\n");
		//testing if this is authorised...

		//if --- has permission on set_acl

	}
	else
		process = true;

	if (process)
		process_permission_change(perms->tree);

	delete perms;
}

void rdc::process_permission_change(snode *permissionnode)
{
	const char  *target_cpt, *target_inst, *target_address;
	int add_perm, check_perm;
	image *img;
	rdcpermission *perm;
	int processed = false;

    //build up the permission object
	perm = new rdcpermission();

	//extract AC rule
	target_cpt = sdup(permissionnode->extract_txt("target_cpt"));
	target_inst = sdup(permissionnode->extract_txt("target_inst"));
	target_address = sdup(permissionnode->extract_txt("target_address")); //note this is ignored by locally specified rules (i.e. by rdc_default.priv)
	perm->target_ept = sdup(permissionnode->extract_txt("target_endpt"));
	perm->principal_cpt = sdup(permissionnode->extract_txt("principal_cpt"));
	perm->principal_inst = sdup(permissionnode->extract_txt("principal_inst"));
    add_perm = permissionnode->extract_flg("add_perm");



    //Is this an RDC rule?
	if (!strcmp(target_cpt,"rdc") || !strcmp(target_cpt,"RDC"))
	{
		//This is a local rule --- privileges handled by the wrapper!
		if (!strcmp("",perm->target_ept))
			com->set_permission(perm->principal_cpt,perm->principal_inst,add_perm);
		else
			com->set_permission(perm->target_ept, perm->principal_cpt,perm->principal_inst,add_perm);
		delete perm;
		return;
	}

    //find the relevant image..
    for (int i = 0; i < live->count(); i++)
    {
    	//printf("testing %s:%s (%s) against %s:%s (%s) --- SIMILAR??? %d\n", live->item(i)->metadata->extract_txt("name"), live->item(i)->state->extract_txt("instance"),live->item(i)->address, target_cpt,target_inst, target_address,live->item(i)->similar(target_cpt,target_inst, target_address));
    	if (live->item(i)->similar(target_cpt,target_inst, target_address))
    	{
    		if (add_perm)
    			live->item(i)->acpolicies->add_permission(perm);
    		else
    			live->item(i)->acpolicies->remove_permission(perm);
    		processed = true;
    	}
    }
    if (!processed)
    {
    	pthread_mutex_lock(&registering_mutex);
    	for (int i = 0; i < registering->count(); i++){
    		if (registering->item(i)->similar(target_cpt,target_inst,target_address))
    		{
    			log("Buffering a permission request for registering component %s:%s (%s) -- for %s:%s adding %d \n",target_cpt,target_inst,target_address, perm->principal_cpt,perm->principal_inst, add_perm);
    			registering->item(i)->buffered_policies->add(new snode(permissionnode));
    		}
    		//printf("we have %s:%s of %s, comp to addr %s\n",registering->item(i)->cpt_name,registering->item(i)->ins_name,registering->item(i)->address,target_address);

    	}
    	pthread_mutex_unlock(&registering_mutex);

    }
    //printf("%successfully Processed.. \n",processed?"S":"UNs");

}



void rdc::lookup(sendpoint *ep)
{
	smessage *query;
	snode *result, *sn_constraints, *sn_interface, *matches, *cpt;
	image *img;
	int count = 0;
	int endpoint_count = 0;
	query = ep->rcv(); // @criteria { ^map-constraints - ^interface - }
	//printf("performing a lookup for %s %s\n",query->source_cpt, query->source_inst);
	sn_constraints = query->tree->extract_item("map-constraints");
	sn_interface = query->tree->extract_item("interface");
	result = mklist("results"); // @results ( txt address )
	pthread_mutex_lock(&live_mutex);
	for(int i = 0; i < live->count(); i++)
	{

		img = live->item(i);
		matches = mklist("endpoints");
		if(img->match(sn_interface, sn_constraints, matches, com, query->source_cpt,query->source_inst)	)
		{
			cpt = mklist("component");
			cpt->append(pack(img->address, "address"));
			cpt->append(matches);
			result->append(cpt);
			endpoint_count += matches->count();
			count++;
		}

	}
	pthread_mutex_unlock(&live_mutex);
	printf("Lookup component returning %d matching endpoint(s) on %d component(s)\n", endpoint_count, count);
	ep->reply(query, result);
	delete query;
}

void rdc::list(sendpoint *ep)
{
	smessage *query;
	snode *result, *sn;
	image *img;
	const char *cpt_name, *instance;


	query = ep->rcv();
	query->source_cpt;
	query->source_inst;
	printf("Doing a list - there are %d components in total\n", live->count());

	result = mklist("cpt-list");
	pthread_mutex_lock(&live_mutex);


	for(int i = 0; i < live->count(); i++)
		{
			img = live->item(i);
			if(img->metadata == NULL || img->state == NULL)
				continue; // No info yet

			cpt_name = img->metadata->extract_txt("name");
			instance = img->state->extract_txt("instance");

			//permission to access to component
			// i.e. if he has access to ANY endpt on this component, he can see it exists..
			if (!img->acpolicies->any_authorising(query->source_cpt, query->source_inst,builtin_names))
				{
					printf("%s:%s is not authorised to access %s:%s - so he will not be informed of its existence...\n",query->source_cpt, query->source_inst, cpt_name, instance);
					continue;
				}

			sn = mklist("component");
			sn->append(pack(img->address, "address"));
			sn->append(pack(cpt_name, "cpt-name"));
			if(strcmp(instance, cpt_name))
				sn->append(pack(instance, "instance"));
			else
				sn->append(pack(SNull, "instance"));

			result->append(sn);
		}
	pthread_mutex_unlock(&live_mutex);
	ep->reply(query, result);
	delete result;
	delete query;
}


void rdc::dump(sendpoint *ep)
{
	smessage *query;
	snode *result, *sn;
	image *img;
	snode *meta, *epts, *search, *status;
	query = ep->rcv();

	result = mklist("rdc-dump");
	pthread_mutex_lock(&live_mutex);
	for(int i = 0; i < live->count(); i++)
		{
			img = live->item(i);
			if(img->metadata == NULL || img->state == NULL || !img->acpolicies->any_authorising(query->source_cpt,query->source_inst,builtin_names))
				continue; // No info yet
			sn = mklist("component");

			sn->append(pack(img->address, "address"));

			//Chop up the metadata - only the authorised endpoints are returned...
			meta = mklist("metadata");
			meta->append(img->metadata->extract_item("name"));
			meta->append(img->metadata->extract_item("description"));
			meta->append(img->metadata->extract_item("keywords"));
			meta->append(img->metadata->extract_item("designer"));
			epts = mklist("endpoints");

			//only add the authorised endpoints
			search = img->metadata->extract_item("endpoints");
			// Loop through all the endpoints specified by the requested interface:
			for(int j = 0; j < search->count(); j++)
			{
				if (img->acpolicies->authorised(search->extract_item(j)->extract_txt("name"), query->source_cpt, query->source_inst))
						 epts->append(search->extract_item(j));
			}
			meta->append(epts);
			sn->append(new snode(meta));

			//now do the same for the state...
			status = mklist("state");
			status->append(img->state->extract_item("address"));
			status->append(img->state->extract_item("instance"));
			status->append(img->state->extract_item("creator"));
			status->append(img->state->extract_item("cmdline"));
			status->append(img->state->extract_item("load"));

			epts = mklist("endpoints");
			search = img->state->extract_item("endpoints");
			for(int j = 0; j < search->count(); j++)
			{
				if (img->acpolicies->authorised(search->extract_item(j)->extract_txt("name"), query->source_cpt, query->source_inst))
						 epts->append(search->extract_item(j));
			}
			status->append(epts);

			status->append(img->state->extract_item("freshness"));
			sn->append(new snode(status));


			result->append(sn);
		}
	pthread_mutex_unlock(&live_mutex);
	ep->reply(query, result);
	delete result;
	delete query;
}

image::image()
{
	address = cpt_name = ins_name = NULL;
	metadata = state = NULL;
	msg_hsh = new svector();
	reply_hsh = new svector();
	msg_hsh_list = mklist("endpoints");
	lost = 0;
	acpolicies = new rdcpermissionstore();
	buffered_policies = new snodevector();
}

void image::init_hashes()
{
	snode *sn, *subn;
	Schema *msg_schema, *reply_schema;
	const char *err;
	char *s;
	int endpoints;
	
	sn = metadata->extract_item("endpoints");
	endpoints = sn->count();
	for(int i = 0; i < endpoints; i++)
	{
		subn = sn->extract_item(i);
		msg_schema = Schema::create(subn->extract_txt("message"), &err);
		if(msg_schema == NULL)
		{
			msg_hsh->add("EEEEEEEEEEEE");
			
			svector *ep = new svector();
			ep->add("EEEEEEEEEEEE");
			snode *sn = pack("EEEEEEEEEEEE", "hash");
			msg_hsh_list->append(sn);
		}
		else
		{
			s = msg_schema->hc->tostring();
			msg_hsh->add(s);
			delete[] s;
			
			// Clone the snode (for after we delete schema) and add to list.
			snode *ep = new snode(msg_schema->hashes);
			msg_hsh_list->append(ep);
		}
		reply_schema = Schema::create(subn->extract_txt("response"), &err);
		if(reply_schema == NULL)
		{
			reply_hsh->add("EEEEEEEEEEEE");
		}
		else
		{
			s = reply_schema->hc->tostring();
			reply_hsh->add(s);
			delete[] s;
		}
		if(msg_schema != NULL) delete msg_schema;
		if(reply_schema != NULL) delete reply_schema;
	}
	// Sanity check:
	if(msg_hsh->count() != endpoints || reply_hsh->count() != endpoints || msg_hsh_list->count() != endpoints)
		error("Number of endpoints assertion failed in RDC");
}

image::~image()
{
	// destroy the checkalive mutex on the image.
	//pthread_mutex_destroy(&checkalive_mutex);
	if(address != NULL) delete[] address;
	if(ins_name != NULL) delete[] ins_name;
	if(cpt_name!= NULL) delete[] cpt_name;
	if(metadata != NULL) delete metadata;
	if(state != NULL) delete state;
	delete msg_hsh;
	delete reply_hsh;
	delete msg_hsh_list;
	delete acpolicies;
	delete buffered_policies;
}

int image::similar(const char *name, const char *instance_name, const char *addr)
{
	return (!strcmp(name,cpt_name) && !strcmp(ins_name, instance_name) && !strcmp(addr,address));
}


int image::hashmatch(const char *hsh1, const char *hsh2)
{
	static const char *poly = "FFFFFFFFFFFF";
	
	if(!strcmp(hsh1, poly) || !strcmp(hsh2, poly))
		return 1;
	if(!strcmp(hsh1, hsh2))
		return 1;
	return 0;
}

int image::match_cpt_metadata(snode *constraints)
{
	snode *sn, *search;
	const char *value;
	
	/* printf("Checking constraints:\n"); constraints->dump(); */
	if(constraints->exists("cpt-name"))
	{
		value = constraints->extract_txt("cpt-name");
		if(strcmp(value, metadata->extract_txt("name")))
			return 0;
	}
	if(constraints->exists("instance-name"))
	{
		value = constraints->extract_txt("instance-name");
		if(strcmp(value, state->extract_txt("instance")))
			return 0;
	}
	if(constraints->exists("creator"))
	{
		value = constraints->extract_txt("creator");
		if(strcmp(value, state->extract_txt("creator")))
			return 0;
	}
	if(constraints->exists("pub-key"))
	{
		value = constraints->extract_txt("pub-key");
		; // Unimplemented
	}
	sn = constraints->extract_item("keywords");
	search = metadata->extract_item("keywords");
	for(int i = 0; i < sn->count(); i++)
	{
		// For each keyword...
		value = sn->extract_txt("keyword");
		int pos;
		for(pos = 0; pos < search->count(); pos++)
			if(!strcmp(value, search->extract_txt(pos)))
				break;
		if(pos == search->count())
			return 0;
	}
	
	return 1;
}

int image::schemamatch(snode *want, snode *have)
{
	// Check if the hashes we want occur ANYWHERE in the hashes we have.
	
	// If there are no constraints, we match.
	if (want->count() == 0) return 1;

	int match = 0;
	
	snode *constraint;
	int exact;
	const char *hash;

	// Loop through all of the hash constraints.
	for (int i = 0; i < want->count(); i++)
	{
		match = 0;
		
		constraint = want->extract_item(i);
		exact = constraint->extract_flg("exact");
		hash = constraint->extract_txt("hash");

		// If 'have' has a hash.
		if (have->exists((exact) ? "has" : "similar"))
		{
			// If it's the hash we want.
			if (!strcmp(hash, have->extract_txt((exact) ? "has" : "similar")))
			{
				// If there a constraints within this one.
				if (constraint->exists("children"))
				{
					if (have->count() == 0)
						match = 0;
					else
					{
						// See if we can match within what we have.
						for (int j = 0; j < have->count(); j++)
						{
							match = schemamatch(constraint->extract_item("children"), have->extract_item(j));
							if (match)
								break;
						}
					}	
				}
				else
					// If there are no constraints within this, we have a match on this.
					match = 1;
				
				// If this matches, move on to the next one.
				if (match)
					continue;
			}
		}

		// Depth first check the children of 'have'.
		for (int j = 0; j < have->count(); j++)
		{
			snode *sn = mklist("schema");
			sn->append(constraint);
			// If constraint is matched somewhere in a child, check next constraint.
			if (schemamatch(sn, have->extract_item(j)))
			{
				match = 1;
				break;
			}
		}
		
		// If we still haven't matched this constraint, we fail.
		if (!match) break;
	}
	
	return match;
}

int image::match(snode *interface, snode *constraints, snode *matches, scomponent *com, const char *principal_cpt, const char *principal_inst)
{
	int match_endpoint_names;
	snode *sn, *search, *subsearch;
	snode *ep_reqd, *ep_actual, *ep_live;
	sbuiltin *bi;
	const char *value;
	int match;
	const char *s;
	
	if (match_cpt_metadata(constraints) == 0)
		return 0;
		
	// Check connected peer component constraints:
	
	sn = constraints->extract_item("parents");
	search = state->extract_item("endpoints");
	for(int i = 0; i < sn->count(); i++)
	{
		// For each required peer constraint...
		value = sn->extract_txt("cpt");
		match = 0;
		for(int j = 0; j < search->count(); j++)
		{
			ep_live = search->extract_item(j);
			subsearch = ep_live->extract_item("peers");
			for(int k = 0; k < subsearch->count(); k++)
			{
				if(!strcmp(value,
						subsearch->extract_item(k)->extract_txt("cpt_name")) ||
					!strcmp(value,
						subsearch->extract_item(k)->extract_txt("instance")))
				{
					match = 1;
					break;
				}
			}
			if(match == 1)
				break;
		}
		if(match == 0)
			return 0;
	}
	sn = constraints->extract_item("ancestors");
	for(int i = 0; i < sn->count(); i++)
	{
		value = sn->extract_txt("cpt");
		; // Unimplemented
	}
	
	// Check interface type compatibility:

	match_endpoint_names = constraints->extract_flg("match-endpoint-names");

	search = metadata->extract_item("endpoints");
	// Loop through all the endpoints specified by the requested interface:
	for(int i = 0; i < interface->count(); i++)
	{
		ep_reqd = interface->extract_item(i);
		match = 0;
		printf(">> Lookup requested for %s:%s:%s by %s %s (endpoint names need not match, it's a schema match)\n", 
			metadata->extract_txt("name"), state->extract_txt("instance"), ep_reqd->extract_txt("name"), principal_cpt, principal_inst);

		// Loop through all this component image's endpoints, to see if satisfy:
		for(int j = 0; j < search->count(); j++)
		{
			ep_actual = search->extract_item(j);
			// Check names, if applicable:
			if(match_endpoint_names && ep_reqd->exists("name") && strcmp(ep_reqd->extract_txt("name"), ep_actual->extract_txt("name")))
					continue;
			// Check types:
			if(strcmp(ep_actual->extract_value("type"), ep_reqd->extract_value("type")) != 0)
			{
				//printf("Endpoint no match on type (needed %s, found %s)\n", ep_reqd->extract_value("type"), ep_actual->extract_value("type"));
				continue;
			}
			
			// This is an snode containing the endpoints fields and their 'has' and 'similar' hashes.
			snode *ept_hashes = msg_hsh_list->extract_item(j);
			
			// Check schema constraints.
			if (constraints->exists("schema"))
			{
				sn = snode::import(constraints->extract_txt("schema"), NULL);
				if (!schemamatch(sn, ept_hashes))
					continue;
			}
			// Let's assume if we're doing a flexible matching (for similar schemas) we don't want the LITMUS tests.
			else
			{
				// OK so far. Now for the LITMUS tests:
				if(!hashmatch(ep_reqd->extract_txt("msg-hash"), msg_hsh->item(j)))
					continue;
				if(!hashmatch(ep_reqd->extract_txt("reply-hash"), reply_hsh->item(j)))
					continue;
				if(!acpolicies->authorised(ep_actual->extract_txt("name"), principal_cpt, principal_inst))
					continue;
			}
			// This endpoint matches, add it to the list.
			matches->append(ep_actual);
		}
		
		// If there are no matches yet, check builtins.
		if(matches->count() > 0) continue;
		
		//printf("testing compatible builtins\n");
		for(int j = 0; j < com->count_builtins(); j++)
		{
			bi = com->get_builtin(j);

			// Check names, if applicable:
			if(match_endpoint_names && ep_reqd->exists("name") && strcmp(ep_reqd->extract_txt("name"), bi->name))
			{
				// printf("Builtin %s didn't match on name\n", bi->name);
				continue;
			}
			// Check types:
			if(strcmp(endpoint_type[bi->type], ep_reqd->extract_value("type")) != 0)
			{

				//printf("Builtin %s no match on type (needed %s, found %s)\n", bi->name, ep_reqd->extract_value("type"), endpoint_type[bi->type]);
				continue;
			}
			// OK so far. Now for the LITMUS tests:
			s = bi->msg_hc->tostring();
			if(!hashmatch(ep_reqd->extract_txt("msg-hash"), s))
				{ delete s; continue; }
			delete s;
			s = bi->reply_hc->tostring();
			if(!hashmatch(ep_reqd->extract_txt("reply-hash"), s))
				{ delete s; continue; }
			delete s;
			//printf("we might have a match with %s for %s -- hash=%s\n",bi->name,ep_reqd->extract_txt("name"),bi->msg_hc->tostring());
			if(!acpolicies->authorised(ep_actual->extract_txt("name"), principal_cpt, principal_inst))
				continue;
				
			match = 1;
			break; // This endpoint matches
		}
		
		if(match) continue;
		return 0; // Couldn't find a match for this interface endpoint
	}

	return 1;
}
