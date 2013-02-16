// component.cpp - DMI - 21-3-07

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <signal.h>
#include <pwd.h>

#include <sys/types.h>
#include <sys/socket.h>

#include "error.h"
#include "datatype.h"
#include "dimension.h"
#include "multiplex.h"
#include "builder.h"
#include "hash.h"
#include "component.h"
#include "lowlevel.h"
#include "net.h"

typedef sendpoint *sendpointptr;
typedef Schema *SchemaPtr;

const char *endpoint_type[] =
{ "server", "client", "source", "sink" };
const int NUM_ENDPOINT_TYPES = 4;

typedef char *charptr;

void wrapper_failed()
{
	error("SBUS Component shutting down because wrapper terminated");
}

/* scomponent */

scomponent::scomponent(const char *cpt_name, const char *instance_name,
		CptUniqueness unique)
{
	name = sdup(cpt_name);
	if(instance_name == NULL)
		instance = sdup(cpt_name);
	else
		instance = sdup(instance_name);
	init_logfile(cpt_name, instance_name, 0);
	epv = new sendpointvector();
	biv = new sbuiltinvector();
	rdc = new svector();
	listen_port = -1; // Not running yet
	uniq = unique;
	canonical_address = NULL;
	wrapper_started = false;
	rdc_update_autoconnect = false;
	rdc_update_notify = false;
	char *rdc_path;
	rdc_path = getenv("SBUS_RDC_PATH");
	if(rdc_path != NULL)
	{
		stringsplit *ss;
		
		ss = new stringsplit(rdc_path, ',');
		for(int i = 0; i < ss->count(); i++)
			rdc->add(ss->item(i));
		delete ss;
	}

	start_wrapper();
}

scomponent::~scomponent()
{
	sendpoint *ep;
	sbuiltin *bi;
	
	stop();
	for(int i = 0; i < epv->count(); i++)
	{
		ep = epv->item(i);
		delete ep;
	}
	delete epv;
	for(int i = 0; i < biv->count(); i++)
	{
		bi = biv->item(i);
		delete bi;
	}
	delete biv;
	delete[] instance;
	delete[] name;
	delete rdc;
	if(canonical_address != NULL) delete[] canonical_address;
}

/* scomponent */

void scomponent::set_log_level(int log, int echo)
{
	log_level = log;
	echo_level = echo;
}

int scomponent::count_endpoints()
{
	return epv->count();
}

const char *scomponent::get_address()
{
	return canonical_address;
}

sendpoint *scomponent::get_endpoint(int index)
{
	if(index < 0 || index >= epv->count())
		error("No such endpoint");
	return epv->item(index);
}

sendpoint *scomponent::get_endpoint(const char *name)
{
	sendpoint *ep;
	
	for(int i = 0; i < epv->count(); i++)
	{
		ep = epv->item(i);
		if(!strcmp(name, ep->name))
			return ep;
	}
	return NULL;
}

sendpoint *scomponent::fd_to_endpoint(int fd)
{
	sendpoint *ep;
	
	for(int i = 0; i < epv->count(); i++)
	{
		ep = epv->item(i);
		if(ep->fd == fd)
			return ep;
	}
	return NULL;
}

sendpoint *scomponent::add_endpoint(const char *name, EndpointType type,
		HashCode *msg_hc, HashCode *reply_hc)
{
	HashCode *msg_hc_copy, *reply_hc_copy;
	
	msg_hc_copy = new HashCode(msg_hc);
	if(reply_hc == NULL)
	{
		reply_hc_copy = new HashCode();
		reply_hc_copy->frommeta(SCHEMA_NA);
	}
	else
		reply_hc_copy = new HashCode(reply_hc);
	
	return do_add_endpoint(name, type, msg_hc_copy, reply_hc_copy);
}
		
sendpoint *scomponent::add_endpoint(const char *name, EndpointType type,
		const char *msg_hash, const char *reply_hash)
{
	HashCode *msg_hc, *reply_hc;

	msg_hc = new HashCode();
	msg_hc->fromstring(msg_hash);
	
	reply_hc = new HashCode();
	if(reply_hash == NULL)
		reply_hc->frommeta(SCHEMA_NA);
	else
		reply_hc->fromstring(reply_hash);
	
	return do_add_endpoint(name, type, msg_hc, reply_hc);
}

sendpoint *scomponent::do_add_endpoint(const char *name, EndpointType type,
		HashCode *msg_hc, HashCode *reply_hc)
{
	// Note: this function consumes its msg_hc and reply_hc arguments
	sendpoint *ep;
	saddendpoint *add;

	/* N.B. if listen_port is not -1, the component has started;
		add_endpoint() is then still allowed, but the endpoint is
		not matched with anything from the component metadata. */
	
	if(!reply_hc->isapplicable() &&
			(type == EndpointClient || type == EndpointServer))
		error("Endpoint requires a response, but reply hash code not set");
	if(reply_hc->isapplicable() &&
			(type == EndpointSource || type == EndpointSink))
		error("Endpoint has no response, but a reply hash code was given");

	ep = new sendpoint(name, type, msg_hc, reply_hc);
	
	// Send MessageAddEndpoint:
	add = new saddendpoint();
	add->endpoint = sdup(name);
	add->type = type;
	add->msg_hc = msg_hc;
	add->reply_hc = reply_hc;
	if(add->write(bootstrap_fd) < 0)
		error("Bootstrap connection to wrapper disconnected in add_endpoint");
	delete add;
	
	ep->fd = acceptsock(callback_fd);
	if(ep->fd < 0)
		error("Couldn't accept additional connection back from wrapper");
	
	// We don't need to delete msg_hc & reply_hc, as consumed by saddendpoint
	epv->add(ep);
	return ep;
}

sendpoint *scomponent::clone(sendpoint *ep)
{
	return add_endpoint(ep->name, ep->type, ep->msg_hc, ep->reply_hc);
}

void scomponent::set_rdc_update_autoconnect(int autoconnect)
{
	rdc_update_autoconnect = autoconnect;
	if (wrapper_started)
		update_rdc_settings();
}

void scomponent::set_rdc_update_notify(int notify)
{
	rdc_update_notify = notify;
	if (wrapper_started)
		update_rdc_settings();
}

sendpoint *scomponent::rdc_update_notifications_endpoint()
{
	const char *ept_name = "rdc_update";
	sendpoint *rdc_update_ep;
	
	rdc_update_ep = get_endpoint(ept_name);
	if (rdc_update_ep == NULL)
	{
		const char *rdc_update_schema = "@event { txt rdc_address flg arrived }";
		HashCode *hc;
	
		hc = declare_schema(rdc_update_schema);
	
		rdc_update_ep = add_endpoint(ept_name, EndpointSink, hc->tostring());
	}
	set_rdc_update_notify(true);
	return rdc_update_ep;
}

HashCode *scomponent::declare_schema(const char *schema)
{
	return do_declare_schema(schema, 0);
}

HashCode *scomponent::load_schema(const char *file)
{
	return do_declare_schema(file, 1);
}

HashCode *scomponent::do_declare_schema(const char *schema, int file_lookup)
{
	shook *hook;
	sgeneric *sg;
	HashCode *hc;

	hook = new shook(MessageDeclare);
	hook->hc = new HashCode();
	hook->hc->fromstring("3D79D04FEBCC");
	// @declare { txt schema flg file_lookup }
	hook->tree = pack(pack(schema, "schema"),
			pack_bool(file_lookup, "file_lookup"), "declare");
		
	if(hook->write(bootstrap_fd) < 0)
		error("Bootstrap connection to wrapper disconnected in load_schema()");
	delete hook;
	
	sg = new sgeneric();
	if(sg->read(bootstrap_fd) < 0)
		error("Error reading schema hash message from wrapper");
	if(sg->type != MessageHash)
		error("Expected schema hash return from wrapper, got type %d", sg->type);
	hc = new HashCode();
	hc->fromstring(sg->tree->extract_txt()); /* @txt hash */
	delete sg;
	
	return hc;
}

const char *scomponent::get_schema(HashCode *hc)
{
	shook *hook;
	sgeneric *sg;
	const char *s;
	const char *hash;
	
	hash = hc->tostring();
	hook = new shook(MessageGetSchema);
	hook->hc = new HashCode();
	hook->hc->fromstring("D3C74D1897A3"); /* @txt hash */
	hook->tree = pack(hash, "hash");
	delete[] hash;

	if(hook->write(bootstrap_fd) < 0)
		error("Bootstrap connection to wrapper disconnected in get_schema()");
	delete hook;
	
	sg = new sgeneric();
	if(sg->read(bootstrap_fd) < 0)
		error("Error reading schema message from wrapper");
	if(sg->type != MessageSchema)
		error("Expected schema return from wrapper, got type %d", sg->type);
	s = sdup(sg->tree->extract_txt()); /* @txt schema */
	delete sg;
	
	return s;
}

snode *scomponent::get_status()
{
	shook *hook;
	sgeneric *status;
	snode *sn;
	
	hook = new shook(MessageGetStatus);
	if(hook->write(bootstrap_fd) < 0)
		error("Bootstrap connection to wrapper disconnected in get_status()");
	delete hook;
	
	status = new sgeneric();
	if(status->read(bootstrap_fd) < 0)
		error("Error reading status message from wrapper");
	if(status->type != MessageStatus)
		error("Expected status return from wrapper");
	sn = status->tree;
	status->tree = NULL;
	delete status;
	
	return sn;
}

void scomponent::stop()
{
	sstopwrapper *packet;
	
	packet = new sstopwrapper();
	packet->reason = 0;
	packet->write(bootstrap_fd);
	delete packet;
	
	// Wait for wrapper to exit (as indicated by closing pipe):
	char buf;
	int bytes;
	
	bytes = read(bootstrap_fd, &buf, 1);
	if(bytes != 0)
		printf("Error: wrapper returned data after being requested to stop\n");
}

void scomponent::add_rdc(const char *address)
{
	if (!wrapper_started)
		rdc->add(address);
	else
		update_rdc_settings(address, 1);	
}

void scomponent::remove_rdc(const char *address)
{
	if (!wrapper_started)
		rdc->remove(address);
	else
		update_rdc_settings(address, 0);
}

void scomponent::update_rdc_settings(const char *address, int arrived)
{
	srdc *rdc_message;

	rdc_message = new srdc();
	rdc_message->address = sdup(address);
	rdc_message->arrived = arrived;
	rdc_message->notify = rdc_update_notify;
	rdc_message->autoconnect = rdc_update_autoconnect;
	
	if(rdc_message->write(bootstrap_fd) < 0)
		error("Cannot write rdc message in add_rdc");
	delete rdc_message;	
}

void scomponent::start(const char *metadata_filename, int port, int register_with_rdc)
{
	class sstartwrapper *start;
	uid_t id;
	struct passwd *pw;

	/*	
	if(epv->count() == 0)
		error("Tried to start a component with no endpoints");
	*/

	start = new sstartwrapper();
	start->cpt_name = sdup(name);
	start->instance_name = sdup(instance);
	
	id = getuid();
	pw = getpwuid(id);
	if(pw == NULL)
		error("Can't lookup user name to set creator field");
	#ifndef __ANDROID__
	start->creator = sdup(pw->pw_gecos);
	#else
	start->creator = sdup("Android");
	#endif
	// Remove trailing commas:
	int pos = 0;
	while(start->creator[pos] != '\0')
	{
		if(start->creator[pos] == ',')
		{
			start->creator[pos] = '\0';
			break;
		}
		pos++;
	}
	
	start->metadata_address = sdup(metadata_filename);
	start->listen_port = ((port < 0) ? 0 : port);
	start->unique = uniq;
	start->log_level = log_level;
	start->echo_level = echo_level;
	start->rdc_register = register_with_rdc;
	start->rdc_update_notify = rdc_update_notify;
	start->rdc_update_autoconnect = rdc_update_autoconnect;
	start->rdc = new svector();
	for(int i = 0; i < rdc->count(); i++)
		start->rdc->add(rdc->item(i));
	if(start->write(bootstrap_fd) < 0)
		error("Bootstrap connection to wrapper disconnected in start");
	delete start;
	
	wrapper_started = true;
	running();
}

void scomponent::running()
{
	// Read srunning:
	srunning *srun;
	snode *builtins, *subn;
	sbuiltin *bi;
	const char *type;
	int index;

	srun = new srunning();
	if(srun->read(bootstrap_fd) < 0)
		error("Error reading srunning message from wrapper");
	// srun->builtins->dump();
	listen_port = srun->listen_port;
	builtins = srun->builtins;
	canonical_address = sdup(srun->address);

	for(int i = 0; i < builtins->count(); i++)
	{
		bi = new sbuiltin;
		subn = builtins->extract_item(i);
		bi->name = sdup(subn->extract_txt("name"));
		type = subn->extract_txt("type");
		for(index = 0; index < NUM_ENDPOINT_TYPES; index++)
		{
			if(!strcmp(endpoint_type[index], type))
			{
				bi->type = (EndpointType)index;
				break;
			}
		}
		if(index == NUM_ENDPOINT_TYPES)
			error("Builtin endpoint has illegal type %s", type);
		bi->msg_hc = new HashCode;
		bi->reply_hc = new HashCode;
		bi->msg_hc->fromstring(subn->extract_txt("msg-hash"));
		bi->reply_hc->fromstring(subn->extract_txt("reply-hash"));
		biv->add(bi);
	}
	delete srun;
}

sbuiltin::sbuiltin()
{
	name = NULL;
	msg_hc = reply_hc = NULL;
}

sbuiltin::~sbuiltin()
{
	if(name != NULL) delete[] name;
	if(msg_hc != NULL) delete msg_hc;
	if(reply_hc != NULL) delete reply_hc;
}

int scomponent::count_builtins()
{
	return biv->count();
}


svector *scomponent::get_builtin_names()
{
	svector *ret = new svector();
	for (int i = 0; i < biv->count(); i++)
		ret->add(sdup(biv->item(i)->name));
	return ret;
}

int scomponent::is_builtin(const char *epname)
{
	for (int i = 0; i < biv->count(); i++)
		if (!strcmp(biv->item(i)->name, epname))
			return true;
	return false;
}

sbuiltin *scomponent::get_builtin(int index)
{
	return biv->item(index);
}

int scomponent::get_listen_port()
{
	return listen_port;
}

void scomponent::start_wrapper()
{
	int pid;
	int callback_port = -1;

	signal(SIGPIPE, SIG_IGN);
	callback_fd = passivesock(&callback_port);	

	pid = fork();
	if(pid < 0)
		error("Cannot create wrapper process.");
	if(pid > 0)
	{
		// Parent process (return to application):
		bootstrap_fd = acceptsock(callback_fd);
		if(bootstrap_fd < 0)
			error("Couldn't accept connection back from wrapper");		
		log("Wrapper (PID %d) running and connected to library", pid);
		return;
	}
	
	// Child process (become the wrapper):
	
	// Close open file descriptors (except stdio):
		for(int i = getdtablesize() - 1; i > 2; i--)
		{
			// printf("Closing file descriptor %d\n", i);
			close(i);
		}

	// Set up some command-line arguments and exec:
	char **newargs;
	newargs = new charptr[3];
	#ifndef __ANDROID__
	newargs[0] = (char *)"sbuswrapper";
	#else
	newargs[0] = (char *)"/data/data/uk.ac.cam.tcs40.sbus.sbus/files/sbuswrapper";
	#endif
	newargs[1] = new char[50];
	sprintf(newargs[1], ":%d", callback_port);
	newargs[2] = NULL;
	debug("Exec invoking wrapper as follows: \"%s %s\"", newargs[0], newargs[1]);
	#ifndef __ANDROID__
	execvp(newargs[0], newargs);
	#else
	execv(newargs[0], newargs);
	#endif
	error("Could not run %s: check it is in your path!", newargs[0]);
}

void scomponent::set_permission(const char *pr_cpt, const char *pr_inst, int authorised)
{
	/*set the privilege for each endpoint
	for(int i = 0; i < epv->count(); i++)
	{
		printf("Component: setting privilege for %s\n",epv->item(i)->name);
		epv->item(i)->set_permission(pr_cpt,pr_inst,add);
	}*/

	if (!wrapper_started)
		{
			warning("Wrapper must have started before changing privilges --- call com->start(..) first!\n");
			return;
		}
	else if (epv->count()<1)
	{
		warning("Cannot communicate with the wrapper - need to specify an endpoint first\n");
		return;
	}

	//choose the first endpoint and send the message through that
	//NB Although v. hacky, in practice permission calls happen after wrapper instantiation, thus this should be safe.
	if (epv->count()>0)
		epv->item(0)->set_permission(pr_cpt, pr_inst, authorised, true);
}

//NB This function actually instructs teh WRAPPER to read in the privileges
// this is because the wrapper already deals with access_control msgs, etc - these details are obscured from the component itself
void scomponent::load_permissions(const char *filename)
{
	//some error checking - prob should check if the file exists.
	if (filename == NULL || strlen(filename)==0)
		return;

	if (!wrapper_started)
	{
		warning("Wrapper must have started before changing privilges --- call com->start(..) first!\n");
		return;
	}
	else if (epv->count()<1)
	{
		warning("Cannot communicate with the wrapper - need to specify an endpoint first\n");
		return;
	}

	epv->item(0)->load_permissions(filename);

}


void scomponent::set_permission(const char *target_ep, const char *pr_cpt, const char *pr_inst, int authorised)
{

	for(int i = 0; i < epv->count(); i++)
		//find the endpoint
		if (!strcmp(target_ep, epv->item(i)->name))
		{
			printf("Component: setting privilege for %s for %s:%s\n",epv->item(i)->name, pr_cpt, pr_inst);
			epv->item(i)->set_permission(pr_cpt,pr_inst,authorised);
		}
		else
			if (i==epv->count())
				warning("Component: Could set permissions on endpoint %s - endpoint not found\n",target_ep);
}

/* sendpoint: */

sendpoint::sendpoint(const char *name, EndpointType type,
		HashCode *msg_hc, HashCode *reply_hc)
{
	this->name = sdup(name);
	this->type = type;
	this->msg_hc = new HashCode(msg_hc);
	this->reply_hc = new HashCode(reply_hc);
	postponed = new smessagequeue();
}

sendpoint::~sendpoint()
{
	if(msg_hc != NULL) delete msg_hc;
	if(reply_hc != NULL) delete reply_hc;
	if(name != NULL) delete[] name;
	// Probably should delete the individual messages on "postponed" here
	delete postponed;
}

void sendpoint::set_permission(const char *pr_cpt, const char *pr_inst, int authorised, int apply_to_all_eps) // = 0)
{
	scontrol *ctrl;
	ctrl = new scontrol();
	ctrl->type = MessagePrivilege;
	if (!apply_to_all_eps)	//nasty hacks.
		ctrl->target_endpoint = sdup(name); //if this is called from the ept - we know it's name...
	ctrl->principal_cpt = sdup(pr_cpt);
	ctrl->principal_inst = sdup(pr_inst);
	ctrl->adding_permission = authorised;
	//printf("created ctrl cpt:...ep='%s' cpt='%s' inst='%s' add='%s' \n", ctrl->target_endpoint, ctrl->principal_cpt, ctrl->principal_inst, ctrl->adding_permission?"ADDING":"REMOVING");
	if(ctrl->write(fd) < 0)
		error("Control connection to wrapper disconnected in permission set");
	delete ctrl;
}

void sendpoint::load_permissions(const char *filename)
{
	scontrol *ctrl;

	ctrl = new scontrol();
	ctrl->type = MessageLoadPrivileges;
	ctrl->filename = sdup(filename);
	printf("Writing to the fd, %s\n",ctrl->filename);
	if(ctrl->write(fd) < 0)
		error("Control connection to wrapper disconnected in permission set");
	delete ctrl;
}

void sendpoint::subscribe(const char *topic, const char *subs, const char *peer)
{
	scontrol *ctrl;

	ctrl = new scontrol();
	ctrl->type = MessageSubscribe;
	ctrl->subs = sdup(subs);
	ctrl->topic = sdup(topic);
	ctrl->peer = sdup(peer); //surely this is required...
	if(ctrl->write(fd) < 0)
		error("Control connection to wrapper disconnected in subscribe");
	delete ctrl;
}


/* sendpoint mapping */

smessage *sendpoint::read_returncode()
{
	// Read map/unmap/ismap return value (may have to buffer incoming data):
	smessage *msg;
	
	while(1)
	{
		msg = new smessage();
		if(msg->read(fd) < 0)
			error("Error reading map success value from wrapper");
		if(msg->type == MessageReturnCode)
			break;
		postponed->add(msg);
	}
	return msg;
}

char *sendpoint::map(const char *address, const char *endpoint, int sflags,
		const char *pub_key)
{
	scontrol *ctrl;
	smessage *msg;
	sreturncode *rc;
	char *s;
	
	ctrl = new scontrol();
	ctrl->type = MessageMap;
	if(endpoint == NULL)
		ctrl->target_endpoint = sdup(name); // Assume same name as other end
	else
		ctrl->target_endpoint = sdup(endpoint);
	ctrl->address = sdup(address);
	if(ctrl->write(fd) < 0)
		error("Control connection to wrapper disconnected in map");
	delete ctrl;	

	msg = read_returncode();
	rc = msg->oob_returncode;
	if(rc->retcode != 1)
	{
		delete msg;
		return NULL; // Map failed
	}
	s = sdup(rc->address);
	delete msg;

	return s; // OK
}

void sendpoint::set_automap_policy(const char *address, const char *endpoint)
{
	scontrol *ctrl;
	
	ctrl = new scontrol();
	ctrl->type = MessageMapPolicy;
	if(endpoint == NULL)
		ctrl->target_endpoint = sdup(name); // Assume same name as other end
	else
		ctrl->target_endpoint = sdup(endpoint);
	ctrl->address = sdup(address);
	if(ctrl->write(fd) < 0)
		error("Control connection to wrapper disconnected in set_automap_policy");
	delete ctrl;
}

/* MapConstraints */

void MapConstraints::init()
{
	cpt_name = instance_name = creator = pub_key = NULL;
	keywords = new svector();
	peers = new svector();
	ancestors = new svector();
	
	match_endpoint_names = 0;
	match_mode = MatchExact;
	tie_policy = TieRandom;
	failed_parse = 0;
}

MapConstraints::MapConstraints()
{
	init();
}

char *MapConstraints::read_word(constcharptr *string)
{
	const char *start, *end;
	char *s;
	
	start = *string;
	end = start;
	while(*end != ' ' && *end != '\t' && *end != '\0' && *end != '+')
		end++;
	s = new char[end - start + 1];
	strncpy(s, start, end - start);
	s[end - start] = '\0';
	*string = end;
	return s;
}

MapConstraints::MapConstraints(const char *string)
{
	char code;
	
	init();
	while(*string != '\0')
	{
		if(*string != '+') { failed_parse = 1; return; }
		string++;
		code = *string;
		string++;
		while(*string == ' ' || *string == '\t')
			string++;
		switch(code)
		{
			case 'N':
				if(cpt_name != NULL) delete[] cpt_name;
				cpt_name = read_word(&string);
				break;
			case 'I':
				if(instance_name != NULL) delete[] instance_name;
				instance_name = read_word(&string);
				break;
			case 'U':
				if(creator != NULL) delete[] creator;
				creator = read_word(&string);
				break;
			case 'X':
				if(pub_key != NULL) delete[] pub_key;
				pub_key = read_word(&string);
				break;
			case 'K':
				keywords->add(read_word(&string));
				break;
			case 'P':
				peers->add(read_word(&string));
				break;
			case 'A':
				ancestors->add(read_word(&string));
				break;
			default: failed_parse = 1; return;
		}
		while(*string == ' ' || *string == '\t')
			string++;
	}
}

MapConstraints::~MapConstraints()
{
	if(cpt_name != NULL) delete[] cpt_name;
	if(instance_name != NULL) delete[] instance_name;
	if(creator != NULL) delete[] creator;
	if(pub_key != NULL) delete[] pub_key;
	delete keywords;
	delete peers;
	delete ancestors;
}

/* is_constraint returns 1 if the string looks like a constraint string,
	0 if it looks like host and/or port, and -1 if it resembles
	neither of these (and hence is definitely an invalid address).
	Note: the string may still fail to pass more detailed parsing checks
	in cases 0 or 1: */

int MapConstraints::is_constraint(const char *string)
{
	int colon = 0, plus = 0;
	while(*string != '\0')
	{
		if(*string == ':')
			colon++;
		else if(*string == '+')
			plus++;
		string++;
	}
	if(colon == 1 && plus == 0)
		return 0;
	if(colon == 0 && plus > 0)
		return 1;
	return -1;
}

snode *MapConstraints::pack()
{
	snode *sn, *subn;

	sn = mklist("map-constraints");
	// The next four lines also work correctly if any string is NULL
	sn->append(::pack(cpt_name, "cpt-name"));
	sn->append(::pack(instance_name, "instance-name"));
	sn->append(::pack(creator, "creator"));
	sn->append(::pack(pub_key, "pub-key"));
	subn = mklist("keywords");
	for(int i = 0; i < keywords->count(); i++)
		subn->append(::pack(keywords->item(i), "keyword"));
	sn->append(subn);
	subn = mklist("parents");
	for(int i = 0; i < peers->count(); i++)
		subn->append(::pack(peers->item(i), "cpt"));
	sn->append(subn);
	subn = mklist("ancestors");
	for(int i = 0; i < ancestors->count(); i++)
		subn->append(::pack(ancestors->item(i), "cpt"));
	sn->append(subn);
	sn->append(::pack_bool(match_endpoint_names, "match-endpoint-names"));
	return sn;
}

void MapConstraints::set_name(const char *s)
{
	if(cpt_name != NULL) delete[] cpt_name;
	cpt_name = sdup(s);
}

void MapConstraints::set_instance(const char *s)
{
	if(instance_name != NULL) delete[] instance_name;
	instance_name = sdup(s);
}

void MapConstraints::set_creator(const char *s)
{
	if(creator != NULL) delete[] creator;
	creator = sdup(s);
}

void MapConstraints::set_key(const char *s)
{
	if(pub_key != NULL) delete[] pub_key;
	pub_key = sdup(s);
}

void MapConstraints::add_keyword(const char *s)
{ keywords->add(s); }

void MapConstraints::add_peer(const char *s)
{ peers->add(s); }

void MapConstraints::add_ancestor(const char *s)
{ ancestors->add(s); }

char *sendpoint::map(MapConstraints *constraints, int sflags)
{
	error("Mapping with constraints structure not implemented yet");
	return NULL; // Failed
}


void sendpoint::unmap()
{
	unmap(NULL,NULL);
}

void sendpoint::unmap(const char *address, const char *endpoint)
{
	scontrol *ctrl;
	smessage *msg;
	
	ctrl = new scontrol();
	ctrl->type = MessageUnmap;
	ctrl->address = sdup(address);
	ctrl->target_endpoint = sdup(endpoint);
	if(ctrl->write(fd) < 0)
		error("Control connection to wrapper disconnected in unmap");
	delete ctrl;

	msg = read_returncode();
	// No data to make use of; just had to wait for it to arrive
	delete msg;
}

int sendpoint::ismapped()
{
	scontrol *ctrl;
	smessage *msg;
	sreturncode *rc;
	int n;
	
	ctrl = new scontrol();
	ctrl->type = MessageIsmap;
	ctrl->address = NULL;
	if(ctrl->write(fd) < 0)
		error("Control connection to wrapper disconnected in ismap");
	delete ctrl;
	
	msg = read_returncode();
	rc = msg->oob_returncode;	
	n = rc->retcode;
	delete msg;

	return n;
}

/* sendpoint data transfer methods */

int sendpoint::message_waiting()
{
	return postponed->count();
}
		
smessage *sendpoint::rcv()
{
	smessage *inc;
	int ret;

	if(!postponed->isempty())
	{
		inc = postponed->remove();
		if(inc->type != MessageRcv)
			error("Incorrect pre-fetched message type in receive from wrapper");
		return inc;
	}	
	inc = new smessage();
	ret = inc->read(fd);
	if(ret == -1)
		wrapper_failed();
	else if(ret < 0)
		error("Error reading incoming data from wrapper");

	if(inc->type == MessageReturnCode)
		error("Return code received, but not within a map/unmap/ismap");
	if(inc->type != MessageRcv)
		error("Incorrect message type in receive from wrapper");
	
	return inc;
}

void sendpoint::reply(smessage *query, snode *result, int exception,
		HashCode *hc)
{
	sinternal *sreq;

	sreq = new sinternal();
	sreq->type = MessageReply;
	sreq->topic = NULL;
	sreq->seq = query->seq;
	if(hc == NULL)
		sreq->hc = new HashCode(reply_hc);
	else
		sreq->hc = new HashCode(hc);
	sreq->xml = result->toxml(0);
	if(sreq->write(fd) < 0)
	{
		wrapper_failed();
		// error("Endpoint data connection to wrapper disconnected in reply");
	}
	
	delete sreq;
}

void sendpoint::emit(snode *sn, const char *topic, HashCode *hc)
{
	sinternal *sreq;

	sreq = new sinternal();
	sreq->type = MessageEmit;
	sreq->topic = sdup(topic);
	sreq->seq = 0;
	if(hc == NULL)
		sreq->hc = new HashCode(msg_hc);
	else
		sreq->hc = new HashCode(hc);
	if(sn == NULL)
	{
		sreq->xml = new char[2];
		sreq->xml[0] = '0';
		sreq->xml[1] = '\0';
	}
	else
		sreq->xml = sn->toxml(0);
	if(sreq->write(fd) < 0)
	{
		wrapper_failed();
		// error("Endpoint data connection to wrapper disconnected in emit");
	}
	
	delete sreq;
}

smessage *sendpoint::rpc(snode *query, HashCode *hc)
{
	sinternal *sreq;
	smessage *msg;
	int ret;

	sreq = new sinternal();
	sreq->type = MessageRPC;
	sreq->topic = NULL;
	sreq->seq = 0;
	if(hc == NULL)
		sreq->hc = new HashCode(msg_hc);
	else
		sreq->hc = new HashCode(hc);
	if(query == NULL)
	{
		sreq->xml = new char[2];
		sreq->xml[0] = '0';
		sreq->xml[1] = '\0';
	}
	else
		sreq->xml = query->toxml(0);
	if(sreq->write(fd) < 0)
	{
		wrapper_failed();
		// error("Endpoint data connection to wrapper disconnected in rpc");
	}
	delete sreq;
	
	// Receive reply:
	
	msg = new smessage();
	ret = msg->read(fd);
	if(ret == -1)
		wrapper_failed();
	else if(ret < 0)
		error("Error reading response from wrapper");
	
	if(msg->type == MessageUnavailable)
	{
		delete msg;
		return NULL;
	}
	else if(msg->type == MessageReturnCode)
		error("Return code received, but not within a map/unmap/ismap");
	else if(msg->type != MessageResponse)
		error("Incorrect message type in response from wrapper");
	
	return msg;
}
