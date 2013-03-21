// wrapper.cpp - DMI - 21-3-07

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <signal.h>
#include <syslog.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <map>

#include "../library/error.h"
#include "../library/datatype.h"
#include "../library/dimension.h"
#include "../library/multiplex.h"
#include "../library/builder.h"
#include "../library/hash.h"
#include "../library/component.h"
#include "../library/lowlevel.h"
#include "../library/net.h"
#include "peernet.h"
#include "express.h"
#include "litmus.h"
#include "marshall.h"
#include "unmarshall.h"
#include "validate.h"
#include "permission.h"
#include "wrapper.h"


swrapper *wrap;
const char *callback_address;

const char *master_schema_file = "cpt_metadata.idl";
const char *status_schema_file = "cpt_status.idl";
const char *lookup_cpt_file = "map_constraints.idl";

const char *map_schema_str =
"@map { txt endpoint txt peer_address txt peer_endpoint txt certificate }";
const char *map_policy_schema_str = "@event { txt endpoint txt peer_address txt peer_endpoint txt certificate flg create }";
const char *unmap_schema_str = "@unmap { txt endpoint [txt peer_address] [txt peer_endpoint] txt certificate }";
const char *divert_schema_str =
"@divert { txt endpoint txt new_address txt new_endpoint [txt peer_address] [txt peer_endpoint] txt certificate }";
const char *subscribe_schema_str =
		"@subscribe { txt endpoint txt peer txt subscription txt topic }";
const char *register_rdc_schema_str = "@event { txt rdc_address flg arrived }";
const char *register_schema_str = "@event { txt address flg arrived }";
const char *lookup_cpt_rep = "@results ( component { txt address endpoints ( endpoint {	txt name @\"cpt_metadata.idl\" ^endpoint-type type ^idl message + response } ) })";
const char *lookup_schema_msg = "@txt hashcode";
const char *lookup_schema_rep = "@txt schema";
const char *set_log_level_msg = "@level { int log int echo }";
const char *lost_schema_str = "@event { txt address flg arrived }";

const char *access_control_schema_str = "@event { txt principal_cpt txt principal_inst [txt target_ept] flg authorised}";

const char *rdc_privilege_schema_str = 	"@event{txt target_cpt txt target_inst txt target_address txt target_endpt txt principal_cpt	txt principal_inst	flg add_perm}";

const char *dump_privileges_reply_schema_str = "  @cpt_privileges{txt name txt instance txt address endpoints ( endpoint{ txt ep_name authorisations( privilege{ txt principal txt instance } ) }) }";

const int nonblocking = 1, nonblockingconnect = 1;

const int timeout_unmap = 1000000; // 1 sec

typedef AbstractMessage *AbstractMessagePtr;
typedef AbstractMessageQueue *AbstractMessageQueuePtr;

void usage()
{
	printf("Usage: wrapper <callback address>\n");
	exit(1);
}

void parse_args(int argc, char **argv)
{
	if(argc != 2)
		usage();
	if(argv[1][0] == '-')
		usage();
	callback_address = argv[1];
}

int main(int argc, char **argv)
{
	parse_args(argc, argv);
	signal(SIGPIPE, SIG_IGN);
	wrap = new swrapper(callback_address);
	wrap->bootstrap();
	wrap->describe();
	wrap->run();
	delete wrap;
	return 0;
}

/* swrapper */

void swrapper::bootstrap()
{
	int ret;
	saddendpoint *add = new saddendpoint();
	sstartwrapper *start = new sstartwrapper();
	char *metadata_filename;

	add_builtin_endpoints();	

	// Listen for endpoint creation requests, and set up new connections:	
	while(1)
	{
		ret = read_startup(bootstrap_fd, add, start);
		if(ret < 0)
			error("Error reading bootstrap messages from library");
		if(ret == 1)
			break;
		add_endpoint(add);
		add->clear();
	}
	delete add;
	// The bootstrap_fd remains in blocking mode, for now:
	// sock_noblock(bootstrap_fd);
	
	// Now process the start message:
	cpt_name = sdup(start->cpt_name);
	instance_name = sdup(start->instance_name);
	init_logfile(cpt_name, instance_name, 1);
	admin = new sadmin();
	admin->creator = sdup(start->creator);
	metadata_filename = sdup(start->metadata_address);
	if(strlen(metadata_filename) == 0)
		error("Metadata filename is empty");
	listen_port = start->listen_port;
	uniq = start->unique;
	register_with_rdc = start->rdc_register;
	rdc_update_notify = start->rdc_update_notify;
	rdc_update_autoconnect = start->rdc_update_autoconnect;
	log_level = start->log_level;
	echo_level = start->echo_level;
	if(listen_port == 0)
		listen_port = -1; // Any port will do

	char *fulladdr;	
	for(int i = 0; i < start->rdc->count(); i++)
	{
		fulladdr = ipaddress::check_add_port(start->rdc->item(i),
				default_rdc_port);
		rdc->add_noduplicates(fulladdr);
		delete[] fulladdr;
	}
	delete start;
	
	char *default_rdc_address = sformat("localhost:%d", default_rdc_port);
	rdc->add_noduplicates(default_rdc_address);
	delete[] default_rdc_address;
	/* Note - this might duplicate an entry already added, in which
		case we will get double lookups and double registration attempts :-( */
	
	// Check component using file metadata:
	Schema *master_schema;
	snode *metadata;
	const char *err;
	char *located;

	master_schema = Schema::load(master_schema_file, &err);
	if(master_schema == NULL)
	{
		error("Error reading master schema from %s:\n%s",
				master_schema_file, err);
	}
	
	located = path_lookup(metadata_filename);
	metadata = snode::import_file(located, &err);
	if(metadata == NULL)
		error("Error reading metadata from %s:\n%s", metadata_filename, err);
	delete[] located;

	if(!validate(metadata, master_schema, &err))
	{
		error("Component metadata from %s does not conform with master "
				"schema:\n%s\n", metadata_filename, err);
	}

	err = verify_metadata(metadata);
	if(err != NULL)
		error("Metadata in file does not match component:\n%s", err);
	delete metadata;
	delete master_schema;
	
	// Find out command line of the component:
	int ppid;
	ppid = getppid();
	cmdline = get_cmdline(ppid);
	log("Component command line: \"%s\"\n", cmdline);
}

char *swrapper::get_cmdline(int pid)
{
	// Access /proc/<pid>/cmdline
	char *procfile, *buf, *s;
	int fd, bytes;
	
	procfile = new char[80];
	buf = new char[500];
	sprintf(procfile, "/proc/%d/cmdline", pid);
	fd = open(procfile, O_RDONLY);
	if(fd == -1)
	{
		warning("Could not open %s", procfile);
		delete[] procfile;
		delete[] buf;
		return sdup("Unknown");
	}
	bytes = read(fd, buf, 500);
	close(fd);
	if(buf[bytes - 1] != '\0')
		error("Overflow reading command line from %s", procfile);
	for(int i = 0; i < bytes - 1; i++)
	{
		if(buf[i] == '\0')
			buf[i] = ' ';
	}
	s = sdup(buf);
	delete[] procfile;
	delete[] buf;
	return s;
}

void swrapper::describe()
{
	smidpoint *mp;
	char *s, *t;

	// printf("Component name '%s' checked against metadata\n", cpt_name);
	log("Wrapper type checks passed for %d endpoints:", mps->count());
	for(int i = 0; i < mps->count(); i++)
	{
		mp = mps->item(i);
		s = mp->msg_hc->tostring();
		if(mp->type == EndpointClient || mp->type == EndpointServer)
			t = mp->reply_hc->tostring();
		else
			t = NULL;
		log("%2d%s. %s \"%s\" msg hash %s%s%s", i, (mp->builtin ? "(BI)" : ""),
				endpoint_type[mp->type], mp->name, s,
				(t == NULL ? "" : " reply hash "),
				(t == NULL ? "" : t));
		if(t != NULL) delete[] t;
		delete[] s;
	}
}

void swrapper::running(int listen_port)
{
	smidpoint *mp;
	srunning *srun;
	snode *builtins, *subn;
	char *s, *t;

	builtins = mklist("interface");
	for(int i = 0; i < mps->count(); i++)
	{
		mp = mps->item(i);
		if(mp->builtin == 0)
			continue;

		s = mp->msg_hc->tostring();
		t = mp->reply_hc->tostring();
		
		subn = mklist("endpoint");
		subn->append(pack(mp->name, "name"));
		subn->append(pack(endpoint_type[mp->type], "type"));
		subn->append(pack(s, "msg-hash"));
		subn->append(pack(t, "reply-hash"));

		delete[] s;
		delete[] t;
		
		builtins->append(subn);
	}
	
	srun = new srunning();
	srun->builtins = builtins;
	srun->listen_port = listen_port;
	srun->address = sdup(canonical_address);
	/*
	printf("Writing this structure:\n");
	builtins->dump();
	*/
	if(srun->write(bootstrap_fd) < 0)
		error("Library disconnected while sending srunning message");
	delete srun;
}

swrapper::swrapper(const char *callback_address)
{
	mps = new smidpointvector();
	multi = new multiplex();
	// Uncomment the next line to enable debugging of file descriptors
	// multi->trace("wrapper-select");
	rdc = new svector();
	cache = new SchemaCache();
	next_uid = 0;
	next_visit = 0;



	bootstrap_fd = activesock(callback_address);
	if(bootstrap_fd < 0)
	{
		error("Can't reach application on callback address '%s'",
				callback_address);
	}
	if(FD_SETSIZE > 1024)
	{
		error("Wasting memory due to large number (%d) of potential file "
				"descriptors", FD_SETSIZE);
	}
	progress_in = new AbstractMessagePtr[FD_SETSIZE];
	progress_out = new AbstractMessageQueuePtr[FD_SETSIZE];
	progress_con = new AbstractMessagePtr[FD_SETSIZE];
	fdstate = new FDState[FD_SETSIZE];
	for(int i = 0; i < FD_SETSIZE; i++)
	{
		progress_in[i] = NULL;
		progress_out[i] = new AbstractMessageQueue();
		progress_con[i] = NULL;
		fdstate[i] = FDUnused;
	}
	fdstate[bootstrap_fd] = FDBoot;
	cmdline = NULL;
}

swrapper::~swrapper()
{
	AbstractMessageQueue *q;
	AbstractMessage *abst;

	for(int i = 0; i < FD_SETSIZE; i++)
	{
		if(progress_in[i] != NULL)
			delete progress_in[i];
		if(progress_con[i] != NULL)
			delete progress_con[i];
		
		q = progress_out[i];
		q->begin();
		abst = q->next();
		while(abst != NULL)
		{
			delete abst;
			abst = q->next();
		}
		delete q;
	}
	delete[] progress_in;
	delete[] progress_out;
	delete[] progress_con;
	delete cache;
	delete rdc;
	delete multi;
	delete mps;
	if(cmdline != NULL)
		delete[] cmdline;
}

void swrapper::set_admin(snode *metadata)
{
	// Copy admin fields:
	snode *sn;
	
	admin->description = sdup(metadata->extract_txt("description"));
	admin->keywords = new svector();
	sn = metadata->extract_item("keywords");
	for(int i = 0; i < sn->count(); i++)
		admin->keywords->add(sn->extract_txt(i));
	admin->designer = sdup(metadata->extract_txt("designer"));
}

const char *swrapper::verify_metadata(snode *metadata)
{
	/* Extract info from metadata and check name and hash sets
		specified agree with component's declared metadata */
	snode *sn;
	smidpoint *mp;
	char *err;
	int num_builtin = 0;
	
	if(strcmp(cpt_name, metadata->extract_txt("name")))
		return "Component name does not match";
	
	set_admin(metadata);

	// Count built-ins:
	for(int i = 0; i < mps->count(); i++)
	{
		mp = mps->item(i);
		if(mp->builtin)
			num_builtin++;
	}
	// N.B. We are now going to assume that all built-in endpoints occur first
	
	// Verify built-in endpoints:
	for(int i = 0; i < num_builtin; i++)
	{
		mp = mps->item(i);
		if(!mp->builtin)
			error("Error: Built-in endpoints should all occur first");
		verify_builtin(mp);
	}

	// Verify custom endpoints:
	sn = metadata->extract_item("endpoints");
	if(sn->count() != mps->count() - num_builtin)
		return "Different number of endpoints";
	for(int i = 0; i < sn->count(); i++)
	{
		mp = mps->item(i + num_builtin);
		err = mp->verify_metadata(sn->extract_item(i));
		if(err != NULL)
			return err;
	}
	// printf("Component metadata verification successful\n");
	return NULL;
}

void swrapper::verify_builtin(smidpoint *mp)
{
	/* Must set msg_schema and reply_schema, and check these hash to the
		same values as msg_hc and reply_hc */
	const char *err;
	
	if(!strcmp(mp->name, "get_metadata"))
	{
		mp->msg_schema = Schema::create("0", &err);
		mp->reply_schema = Schema::load(master_schema_file, &err);
	}
	else if(!strcmp(mp->name, "get_status"))
	{
		mp->msg_schema = Schema::create("0", &err);
		mp->reply_schema = Schema::load(status_schema_file, &err);
	}
	else if(!strcmp(mp->name, "map"))
	{
		mp->msg_schema = Schema::create(map_schema_str, &err);
		mp->reply_schema = Schema::create("!", &err);
	}
	else if(!strcmp(mp->name, "unmap"))
	{
		mp->msg_schema = Schema::create(unmap_schema_str, &err);
		mp->reply_schema = Schema::create("!", &err);
	}
	else if(!strcmp(mp->name, "divert"))
	{
		mp->msg_schema = Schema::create(divert_schema_str, &err);
		mp->reply_schema = Schema::create("!", &err);
	}
	else if(!strcmp(mp->name, "subscribe"))
	{
		mp->msg_schema = Schema::create(subscribe_schema_str, &err);
		mp->reply_schema = Schema::create("!", &err);
	}
	else if(!strcmp(mp->name, "register_rdc"))
	{
		mp->msg_schema = Schema::create(register_rdc_schema_str, &err);
		mp->reply_schema = Schema::create("!", &err);
	}
	else if(!strcmp(mp->name, "register"))
	{
		mp->msg_schema = Schema::create(register_schema_str, &err);
		mp->reply_schema = Schema::create("!", &err);
	}
	else if(!strcmp(mp->name, "lookup_cpt"))
	{
		mp->msg_schema = Schema::load(lookup_cpt_file, &err);		
		mp->reply_schema = Schema::create(lookup_cpt_rep, &err);
	}
	else if(!strcmp(mp->name, "lookup_schema"))
	{
		mp->msg_schema = Schema::create(lookup_schema_msg, &err);
		mp->reply_schema = Schema::create(lookup_schema_rep, &err);
	}
	else if(!strcmp(mp->name, "set_log_level"))
	{
		mp->msg_schema = Schema::create(set_log_level_msg, &err);
		mp->reply_schema = Schema::create("!", &err);
	}
	else if(!strcmp(mp->name, "terminate"))
	{
		mp->msg_schema = Schema::create("0", &err);
		mp->reply_schema = Schema::create("!", &err);
	}
	else if(!strcmp(mp->name, "lost"))
	{
		mp->msg_schema = Schema::create(lost_schema_str, &err);
		mp->reply_schema = Schema::create("!", &err);
	}
	else if (!strcmp(mp->name, "map_policy"))
	{
		mp->msg_schema = Schema::create(map_policy_schema_str, &err);
		mp->reply_schema = Schema::create("!", &err);
	}
	else if(!strcmp(mp->name, "access_control"))
	{
		mp->msg_schema = Schema::create(access_control_schema_str, &err);
		mp->reply_schema = Schema::create("!", &err);
	}
	else if(!strcmp(mp->name, "dump_privileges"))
	{
		mp->msg_schema = Schema::create("0", &err);
		mp->reply_schema = Schema::create(dump_privileges_reply_schema_str, &err);
	}
	else if(!strcmp(mp->name, "rdc_privs"))
	{
		mp->msg_schema = Schema::create(rdc_privilege_schema_str, &err);
		mp->reply_schema = Schema::create("!", &err);
	}
	else
		error("Unknown built-in endpoint '%s' in verify_builtin()", mp->name);

	if(mp->msg_schema == NULL)
		error("Impossible error creating schema for built-in '%s'", mp->name);
	if(mp->reply_schema == NULL)
		error("Impossible error creating schema for built-in '%s'", mp->name);
		
	// Check hash values:	
	if(!mp->msg_hc->equals(mp->msg_schema->hc))
		error("Built-in endpoint '%s' has wrong message schema hash", mp->name);
	if(!mp->reply_hc->equals(mp->reply_schema->hc))
		error("Built-in endpoint '%s' has wrong reply schema hash", mp->name);
	
	cache->add(mp->msg_schema);
	cache->add(mp->reply_schema);
}

int smidpoint::compatible(const char *required_endpoint, int required_ep_id,
		EndpointType type, HashCode *msg_hc, HashCode *reply_hc, int flexible_matching)
{
	if(this->type != type)
	{
		// printf("Incompatible type\n");
		return 0;
	}
	if(required_endpoint != NULL && strcmp(name, required_endpoint))
	{
		// printf("Incompatible endpoint names\n");
		return 0;
	}
	if(required_ep_id > 0 && ep_id != required_ep_id)
	{
		// printf("Incompatible endpoint ID number\n");
		return 0;
	}

	// Check hashes match, or that we're going to do partial matching.
	// If partial matching, we'll connect as normal - the side wanting to do it will have to sort everything out.
	if(!flexible_matching && !msg_hc->ispolymorphic() && !this->msg_hc->ispolymorphic() && !msg_hc->equals(this->msg_hc))
	{
		// printf("Incompatible message hash code\n");
		return 0;
	}
	if(!flexible_matching && !reply_hc->ispolymorphic() && !this->reply_hc->ispolymorphic() && !reply_hc->equals(this->reply_hc))
	{
		// printf("Incompatible reply hash code\n");
		return 0;
	}
	return 1;
}

void swrapper::serve_visitor(int fd, shello *hello)
{
	// Handshake simulation; convert svisitor to scomm; handover to serve_peer
	svisitor *visitor;
	scomm *com;
	AcceptanceCode code;
	smidpoint *mp;
	int index;
	speer *peer;

	visitor = hello->oob_visitor;
	hello->oob_visitor = NULL;
	delete hello;
	// Check we're the right component (target NULL means don't care):
	code = AcceptOK;
	if(visitor->tgt_cpt != NULL && strcmp(visitor->tgt_cpt, cpt_name))
		code = AcceptWrongCpt;

	//TODO: THis is for temporary conns:
	//check they have access to the cpt..

	//for debugging - just sent the code to wrong!
	//code = AcceptNoAccess;
	//printf("Temporary msg received\n");


	for(index = 0; index < mps->count(); index++)
	{
		mp = mps->item(index);
		//check if authorised

		if(mp->compatible(visitor->required_endpoint, 0, visitor->ep_type, visitor->msg_hc, visitor->reply_hc))
			{
				//compatible, but do they have access?
				log("Serving visitor %s:%s requesting endpoint %s:'%s' -- ",visitor->src_cpt,visitor->src_instance,cpt_name,visitor->required_endpoint);
				if (!mp->acl_ep->check_authorised(visitor->src_cpt,visitor->src_instance))
					code = AcceptNoAccess;
				//TODO: Exiting now assumes only ONE compatible endpoint.
				// what if a component has multiple...
				break;
			}
	}
	if(index == mps->count())
		code = AcceptNoEndpoint;

	if(code == AcceptOK || code == AcceptNoAccess)
	{
		// Create temporary peer structure:
		peer = new speer();
		peer->disposable = 1;
		peer->uid = next_uid++;
		peer->sock = fd;
		peer->cpt_name = sdup(visitor->src_cpt);
		peer->instance = sdup(visitor->src_instance);
		peer->endpoint = new char[20];
		sprintf(peer->endpoint, "visitor%d", visitor->dispose_id);
		peer->address = NULL; // Does this matter?
		peer->subs = NULL;
		peer->topic = NULL;
		peer->owner = mp;
		peer->msg_poly = visitor->msg_hc->ispolymorphic();
		peer->reply_poly = visitor->reply_hc->ispolymorphic();
		mp->peers->add(peer);

		log("Accepted disposable connection from peer component %s instance %s",
				peer->cpt_name, peer->instance);

		fdstate[fd] = FDPeer;
		com = new scomm(visitor);

		//nasty hack, see the todo below
		if(code==AcceptNoAccess)
		{
			warning("Warning: rejected disposable connection from %s, code %s",visitor->src_cpt, acceptance_code[code]);
			com->terminate_disposable = true;
		}
		serve_peer(com, peer);
	}
	else
	{
		//TODO - If we reject a connection here - closing the fd crashes everything (per Ingram), as with simply retuning.
		// I have the feeling that we haven't cleared the incoming 'msg' before exiting, so we get stuck in a loop of processing the incoming request...
		//* ---- hacked up for now (see above...)
		warning("Warning: rejected disposable connection from %s, code %s",
				visitor->src_cpt, acceptance_code[code]);

		//close(fd); -- old code, but incorrect.
	}

	delete visitor;
}

void swrapper::begin_accept()
{
	int dyn_sock;
	AbstractMessage *abst;
	
	dyn_sock = acceptsock(external_master_sock);
	if(nonblocking)
		sock_noblock(dyn_sock);
	
	// Preselect to read the hello message:
	abst = new AbstractMessage(dyn_sock);
	preselect_fd->add(dyn_sock);
	preselect_direction->add(FDInbound);
	multi->add(dyn_sock, MULTI_READ, "swrapper::begin_accept");
	progress_in[dyn_sock] = abst;
	fdstate[dyn_sock] = FDAccepted;
}

void swrapper::do_accept(int dyn_sock, AbstractMessage *abst)
{
	speer *peer;
	shello *hello;
	swelcome *welcome;
	smidpoint *mp;
	AcceptanceCode code;
	int index;
	int ret;

	hello = new shello();
	ret = hello->reveal(abst);
	delete abst;
	if(ret < 0)
	{
		warning("Warning: First message on new connection has wrong type; "
				"dropping connection");
		delete hello;
		close(dyn_sock);
		return;
	}

	// Check for disposable connection:
	if(hello->oob_visitor != NULL)
	{
		serve_visitor(dyn_sock, hello);
		return;
	}


	// Check we're the right component (target NULL means don't care):
	code = AcceptOK;
	if(hello->target != NULL && strcmp(hello->target, cpt_name))
		code = AcceptWrongCpt;
	//TODO: THis is for temporary conns:
		//check they have access to the cpt..
	for(index = 0; index < mps->count(); index++)
	{
		mp = mps->item(index);
		//check if authorised

		if(mp->compatible(hello->required_endpoint, hello->required_ep_id, hello->target_type, 
											hello->msg_hc, hello->reply_hc, hello->flexible_matching))
			{
				//compatible, but do they have access?
				log("Serving hello %s:%s requesting endpoint '%s' ", hello->source, hello->from_instance, hello->required_endpoint);
				if (!mp->acl_ep->check_authorised(hello->source, hello->from_instance))
					code = AcceptNoAccess;
				//TODO: Exiting now assumes only ONE compatible endpoint.
				// what if a component has multiple...
				break;
			}
	}
	if(index == mps->count())
		code = AcceptNoEndpoint;

	// Check if already mapped...
	for(int i = 0; i < mp->peers->count(); i++)
	{
		peer = mp->peers->item(i);
		if(!strcmp(peer->address, hello->from_address) && !strcmp(peer->endpoint, hello->from_endpoint))
		{
			code = AcceptAlreadyMapped;
			break;
		}
	}
	
	if(code == AcceptOK)
	{
		// Precompute peer object ready for use after handshake finishes:
		peer = new speer();
		peer->uid = next_uid++;
		peer->sock = dyn_sock;
		peer->cpt_name = sdup(hello->source);
		peer->instance = sdup(hello->from_instance);
		peer->endpoint = sdup(hello->from_endpoint);
		peer->ep_id = hello->from_ep_id;
		peer->address = sdup(hello->from_address);
		if(hello->subs == NULL || mp->msg_hc->equals(hello->msg_hc) == 0)
			peer->subs = NULL;
		else
		{
			peer->subs = new subscription(hello->subs);
			/*
			printf("Parsed subscription:\n");
			peer->subs->dump_tokens();
			peer->subs->dump_tree();
			*/
		}
		peer->topic = sdup(hello->topic);
		peer->owner = mp;
		peer->msg_poly = hello->msg_hc->ispolymorphic();
		peer->reply_poly = hello->reply_hc->ispolymorphic();
		
		log("Accepted connection from peer component %s, endpoint %s",
				peer->cpt_name, peer->endpoint);
		log("(Asked for ep '%s' of type %s, hash %s / %s)",
				((hello->required_endpoint == NULL) ? "any" :
				hello->required_endpoint), endpoint_type[hello->target_type],
				hello->msg_hc->tostring(), hello->reply_hc->tostring());
	}
	else
	{
		warning("Rejected new connection from peer component %s, code %d (%s)",
				hello->source, code, acceptance_code[code]);
		warning("(Asked for endpoint '%s' of type %s, hash %s / %s)",
				((hello->required_endpoint == NULL) ? "any" :
				hello->required_endpoint), endpoint_type[hello->target_type],
				hello->msg_hc->tostring(), hello->reply_hc->tostring());
		peer = NULL;
	}
	
	// Form welcome message:
	welcome = new swelcome();
	welcome->code = code;
	welcome->cpt_name = sdup(cpt_name);
	welcome->instance = sdup(instance_name);
	welcome->address = sdup(canonical_address);
	if(code == AcceptOK)
	{
		welcome->endpoint = sdup(mp->name);
		welcome->subs = ((mp->subs == NULL) ? NULL : sdup(mp->subs));
		welcome->topic = ((mp->topic == NULL) ? NULL : sdup(mp->topic));
		welcome->msg_poly = mp->msg_hc->ispolymorphic();
		welcome->reply_poly = mp->reply_hc->ispolymorphic();
		welcome->msg_hc = (mp->msg_hc == NULL) ? new HashCode() : new HashCode(mp->msg_hc);
		welcome->reply_hc = (mp->reply_hc == NULL) ? new HashCode() : new HashCode(mp->reply_hc);
	}
	else
	{
		welcome->endpoint = welcome->subs = welcome->topic = NULL;
		welcome->msg_hc = new HashCode();
		welcome->reply_hc = new HashCode();
	}
	
	abst = welcome->wrap(dyn_sock);
	delete welcome;
	delete hello;

	// Preselect to send the welcome message:
	abst->peer = peer;
	preselect_fd->add(dyn_sock);
	preselect_direction->add(FDOutbound);
	multi->add(dyn_sock, MULTI_WRITE, "swrapper::do_accept");
	AbstractMessageQueue *q = progress_out[dyn_sock];
	sassert(q->isempty(), "progress_out queue not empty");
	q->add(abst);
	fdstate[dyn_sock] = FDWelcoming;
}

void swrapper::finalise_accept(int dyn_sock, speer *peer)
{
	if(peer != NULL)
	{
			// Handshake complete; officially add pre-prepared peer object:
		smidpoint *mp = peer->owner;
		
		mp->peers->add(peer);
		multi->add(peer->sock, MULTI_READ, "swrapper::finalise_accept");
		fdstate[peer->sock] = FDPeer;
	}
	else
	{
		multi->remove(dyn_sock, MULTI_READ);
		fdstate[dyn_sock] = FDUnused;
	}
}

void swrapper::lost(speer *peer)
{
	const char *rdc_address;
	snode *sn;
	int ok;
	const char *addr = peer->address;
	const char *cpt = peer->cpt_name;
	const char *instance = peer->instance;

	//TODO: Should NON-RDC registered components report losses? for now, let's say no...
	//if (!register_with_rdc)
	//	return;

	if(instance != NULL && strcmp(cpt, instance))
	{
		log("Lost contact with instance %s of component %s at %s; "
				"informing RDCs", instance, cpt, addr);
	}
	else
	{
		log("Lost contact with component %s at %s; informing RDCs", cpt, addr);
	}
	sn = pack(pack(addr, "address"), pack_bool(0, "arrived"), "event");

	for(int i = 0; i < rdc->count(); i++)
	{
		rdc_address = rdc->item(i);
		if(rdc_address[0] == '!')
			continue;
		ok = begin_visit(VisitLost, rdc_address, "rdc", "lost",
				lost_mp, sn, NULL, NULL, NULL);
		if(ok < 0)
			log("Note: could not send lost message to RDC at %s", rdc_address);
	}
}

void swrapper::register_cpt(int arrive, const char *address)
{
	const char *rdc_address;
	snode *sn;
	int ok;
	registerparams *params;
	
	if(!strcmp(cpt_name, "rdc") && !strcmp(instance_name, "rdc"))
		return; // RDC's themselves don't get registered
	
	// Report back host address and port number:
	if(arrive)
		log("local address: %s", canonical_address);
	sn = pack(pack(canonical_address, "address"),
			pack_bool(arrive, "arrived"), "event");

	params = new registerparams();
	params->arrive = arrive;
	if (address == NULL)
		params->count = rdc->count();
	else
		params->count = 1;
	params->failed = params->succeeded = 0;

	if (address == NULL)
	{
		for(int i = 0; i < rdc->count(); i++)
		{
			rdc_address = rdc->item(i);
			if(rdc_address[0] == '!')
				continue;
			ok = begin_visit(VisitRegister, rdc_address, "rdc", "register",
					 register_mp, sn, NULL, params, NULL);

			if(ok < 0)
			{
				params->failed++;
				if(arrive)
					log("Note: could not register with RDC at %s", rdc_address);
			}
			else
			{
				/* OK so far, but a non-blocking connect may well still fail,
					so this doesn't count as a success yet */
			}
		}
	}
	else
	{
		rdc_address = address;
		if(rdc_address[0] != '!')
		{
			ok = begin_visit(VisitRegister, rdc_address, "rdc", "register",
					 register_mp, sn, NULL, params, NULL);

			if(ok < 0)
			{
				params->failed++;
				if(arrive)
					log("Note: could not register with RDC at %s", rdc_address);
			}
			else
			{
				/* OK so far, but a non-blocking connect may well still fail,
					so this doesn't count as a success yet */
			}
		}
	}
	if(params->failed == params->count)
	{
		// All failed at the first hurdle, i.e. initial connection
		if(arrive)
		{
			if (address == NULL)
				warning("Warning: no RDCs available to register with");
			else
				warning("Warning: could not register with RDC %s", address);
			//couldnt find the RDC, so let's disconnect
			register_with_rdc = false;
			setdefaultprivs();
		}
		else
		{
			// warning("Note: no RDCs available to deregister from");
			
		}
		delete params;
	}
	// Otherwise, wait and see...
}

void swrapper::handle_new_rdc(int arrive, const char *address)
{
	if (arrive)
	{
		// add the rdc if it is not already.
		rdc->add_noduplicates(address);

		// register the component with the rdc		
		register_cpt(1, address);

		// need to send all permissions we have.
		smidpoint *mp;
		privilegeparamsvector *privilegevector = new privilegeparamsvector();
		spermission *permission;
		
		// Create a list of all permissions to be sent.
		for (int i = 0; i < mps->count(); i++)
		{
			mp = mps->item(i);
			for (int j = 0; j < mp->acl_ep->count(); j++)
			{
				permission = mp->acl_ep->item(j);
				privilegevector->add(new privilegeparams(mp->name, permission->principal_cpt, permission->principal_inst, true));
			}
		}
		
		// Send permissions to new RDC only, and clean up.
		privilegeparams *privs;
		while ((privs = privilegevector->pop()) != NULL)
		{
			update_privileges_on_rdc(privs, address);
			delete privs;
		}
		
		delete privilegevector;
		
	}
	else
	{
		// deregister from rdc and remove the rdc from our list.
		// Note: we may still be registered (cannot send message because left the network).
		// The rdc will detect this if we leave the network.
		register_cpt(0, address);

		// don't remove localhost
		if (strcmp(address, "localhost:50123"))
			rdc->remove(address);
	}
}

void swrapper::departure(speer *peer, int unexpected)
{
	smidpoint *mp = peer->owner;
	
	multi->remove(peer->sock, MULTI_READ);
	/* Don't print a warning if the MULTI_WRITE fd isn't in the set
		(it probably isn't, if there's no partial write in progress): */
	multi->remove(peer->sock, MULTI_WRITE, 1);
	if(close(peer->sock) < 0)
		warning("Error on close()");
	progress_in[peer->sock] = NULL;
	progress_con[peer->sock] = NULL;
	fdstate[peer->sock] = FDUnused;

	if(unexpected)
		lost(peer);
	
	if(mp->type == EndpointClient)
	{
		// Server has gone - cancel all issued_rpcs:
		/* Eventually we will be able to hold them to be re-sent to another/
			new replica of course, so the application doesn't notice */
		smessage *msg;
		
		while(!mp->issued_rpcs->isempty())
		{
			msg = mp->issued_rpcs->remove();
			// Send back to library as a MessageUnavailable:
			msg->type = MessageUnavailable;
			mp->deliver_local(msg);
			delete msg;
		}
	}
	else if(mp->type == EndpointServer)
	{
		// Client has gone - drop any pending_replies for /this/ client:
		scomm *com;

		mp->pending_replies->begin();
		com = mp->pending_replies->next();
		while(com != NULL)
		{
			if(com->peer_uid == peer->uid)
			{
				delete com;
				com = mp->pending_replies->unlink_and_next();
			}
			else
				com = mp->pending_replies->next();
		}		
	}
				
	// Check if any deferred actions now need to be aborted
	;
	
	AbstractMessageQueue *q = progress_out[peer->sock];
	AbstractMessage *abst = q->remove();
	while(abst != NULL) { delete abst; abst = q->remove(); }
	
	mp->peers->remove(peer);
	delete peer;
}

void swrapper::continue_welcome(int fd)
{
	int complete;
	AbstractMessage *abst;
	AbstractMessageQueue *q;

	q = progress_out[fd];
	sassert(q->count() == 1,
			"Welcome progress_out queue does not contain 1 element");
	abst = q->preview();
	complete = abst->advance();
	if(complete == 1)
	{
		multi->remove(fd, MULTI_WRITE);
		q->remove();
		finalise_accept(fd, abst->peer);
		delete abst;
	}
	else if(complete < 0)
	{
		warning("Connection to peer disconnected whilst sending welcome");
		multi->remove(fd, MULTI_WRITE);
		q->remove();
		if(abst->peer != NULL)
			delete abst->peer;
		delete abst;
		if(close(fd) < 0)
			warning("Error on close()");
		fdstate[fd] = FDUnused;
	}
}

void swrapper::continue_accept(int fd)
{
	int complete;
	AbstractMessage *abst;
	
	abst = progress_in[fd];
	complete = abst->advance();
	if(complete == 1)
	{
		progress_in[fd] = NULL;
		do_accept(fd, abst);
	}
	else if(complete < 0)
	{
		warning("Warning: Error reading first message from new "
				"connection; dropping connection");
		fdstate[fd] = FDUnused;
		progress_in[fd] = NULL;
		multi->remove(fd, MULTI_READ);
		delete abst;
		if(close(fd) < 0)
			warning("Error on close()");
	}
}

void swrapper::continue_greeting(int fd)
{
	int complete;
	AbstractMessage *abst;
	AbstractMessageQueue *q;

	q = progress_out[fd];
	sassert(q->count() == 1,
			"Greeting progress_out queue does not contain 1 element");
	abst = q->preview();
	complete = abst->advance();
	if(complete == 1)
	{
		speer *peer = abst->peer;
		int report_fd = abst->report_fd;

		multi->remove(fd, MULTI_WRITE);
		q->remove();
		delete abst;

		// Preselect to read the welcome message:
		abst = new AbstractMessage(fd);
		abst->peer = peer;
		abst->report_fd = report_fd;
		preselect_fd->add(fd);
		preselect_direction->add(FDInbound);
		multi->add(fd, MULTI_READ, "swrapper::continue_greeting");
		progress_in[fd] = abst;
		fdstate[fd] = FDPending;
	}
	else if(complete < 0)
	{
		int report_fd = abst->report_fd;
		
		warning("Warning: Connection to peer disconnected when sending hello "
				"message");
		
		multi->remove(fd, MULTI_WRITE);
		q->remove();
		if(abst->peer != NULL)
			delete abst->peer;
		delete abst;
		
		map_report(report_fd, 0);
		if(close(fd) < 0)
			warning("Error on close()");
		fdstate[fd] = FDUnused;
	}
}

void swrapper::continue_approval(int fd)
{
	int complete;
	AbstractMessage *abst;

	abst = progress_in[fd];
	complete = abst->advance();
	if(complete == 1)
	{
		progress_in[fd] = NULL;
		finalise_map(fd, abst);
	}
	else if(complete < 0)
	{
		int report_fd = abst->report_fd;
		
		warning("Warning: error reading welcome message from new mapping; "
				"dropping connection");
		fdstate[fd] = FDUnused;
		progress_in[fd] = NULL;
		multi->remove(fd, MULTI_READ);		
		if(close(fd) < 0)
			warning("Error on close()");
		delete abst->peer;
		delete abst;
		
		map_report(report_fd, 0);
		return;
	}
}

void swrapper::continue_visit(int fd)
{
	int complete;
	AbstractMessage *abst;
	AbstractMessageQueue *q;

	q = progress_out[fd];
	sassert(q->count() == 1,
			"Visitor progress_out queue does not contain 1 element");
	abst = q->preview();
	complete = abst->advance();
	if(complete == 1)
	{
		VisitPurpose purpose;
		Schema *reply_schema;
		mapparams *map_params;
		registerparams *reg_params;
		scomm *unrecognised;
		char *address;
		
		purpose = abst->purpose;
		reply_schema = abst->reply_schema;
		map_params = abst->map_params;
		reg_params = abst->reg_params;
		unrecognised = abst->unrecognised;
		address = sdup(abst->address);
		
		multi->remove(fd, MULTI_WRITE);
		q->remove();

		if(purpose == VisitRegister && reg_params->arrive == 0)
		{
			/* Deregistration prior to wrapper termination: now that we've
				got the deregistration message out to at least one RDC,
				we exit without further ado: */
			//exit(0);
		}
		
		if(reply_schema == NULL)
		{
			// No reply to wait for; close disposable connection
			
			finalise_sink_visit(abst);
			delete abst;
			
			if(close(fd) < 0) error("Error in close()");
			fdstate[fd] = FDUnused;
			return;
		}
		delete abst;

		// Preselect to read reply			
		abst = new AbstractMessage(fd);
		abst->purpose = purpose;
		abst->reply_schema = reply_schema;
		abst->map_params = map_params;
		abst->reg_params = reg_params;
		abst->unrecognised = unrecognised;
		abst->address = address;
		
		preselect_fd->add(fd);
		preselect_direction->add(FDInbound);
		multi->add(fd, MULTI_READ, "swrapper::continue_visit");
		progress_in[fd] = abst;
		fdstate[fd] = FDDisposing;
	}
	else if(complete < 0)
	{
		warning("Warning: peer %s disconnected while we sent "
				"visitor message", abst->address);
		
		multi->remove(fd, MULTI_WRITE);
		q->remove();		
		if(close(fd) < 0)
			warning("Error on close()");
		fdstate[fd] = FDUnused;

		// Any special actions due to aborting visit go here:
		abort_visit(abst);
		
		delete abst;
	}
}

void swrapper::continue_dispose(int fd)
{
	int complete;
	AbstractMessage *abst;
	abst = progress_in[fd];
	complete = abst->advance();
	if(complete == 1)
	{
		fdstate[fd] = FDUnused;
		progress_in[fd] = NULL;
		multi->remove(fd, MULTI_READ);		
		if(close(fd) < 0)
			warning("Error on close()");
		
		finalise_server_visit(abst);
	}
	else if(complete < 0)
	{
		warning("Warning: error reading reply from disposable connection; "
				"dropping connection");
		
		fdstate[fd] = FDUnused;
		progress_in[fd] = NULL;
		multi->remove(fd, MULTI_READ);		
		if(close(fd) < 0)
			warning("Error on close()");
		
		// Any special actions due to aborting visit go here:
		abort_visit(abst);
		
		delete abst;
	}
}

void swrapper::continue_write(int fd, speer *peer)
{
	// fd has become writable...
	AbstractMessageQueue *q;
	AbstractMessage *abst;
	int complete;
	
	q = progress_out[fd];
	if(q->isempty())
	{
		/* A fd on which we are not currently trying to write anything
			has become writable: this isn't an event which interests us */
		multi->remove(fd, MULTI_WRITE);
		return;
	}
	abst = q->preview();
	complete = abst->advance();
	if(complete == 0)
	{
		// Still didn't manage to send the whole message:
		return;
	}
	else if(complete == -1)
	{
		// Disconnected:
		if(peer != NULL) // External connection
		{
			warning("Warning: Peer disconnected unexpectedly, "
					"removing it from list");
			departure(peer, 1);
			// No need to delete abst, as departure() will have done it
		}
		else // Internal connection
			error("Internal data connection to library disconnected");
	}
	else if(complete == -2)
	{
		error("Impossible: AbstractMessage cannot generate "
				"protocol errors on outbound data");
	}
	else
	{
		// Complete message sent:
		q->remove();
		if(q->isempty())
			multi->remove(fd, MULTI_WRITE);
		
		if(peer != NULL && (peer->disposable || abst->diverting))
			departure(peer, 0);

		delete abst;
	}
}

void swrapper::continue_read(int fd, smidpoint *mp, speer *peer)
{
	AbstractMessage *abst;
	int complete;

		// fd has become readable...
	abst = progress_in[fd]; // Continue any partial read
	if(abst == NULL)		
		abst = new AbstractMessage(fd);
	complete = abst->advance();
	if(complete == 0)
	{
		// Message transfer incomplete; record this to resume later:
		progress_in[fd] = abst;
		log("Read incomplete message");
	}
	else if(complete == -1)
	{
		// Disconnected:
		if(peer != NULL) // External connection
		{
			warning("Warning: Peer disconnected unexpectedly, "
					"removing it from list");
			departure(peer, 1);
			delete abst;
		}
		else // Internal connection
			error("Library disconnected endpoint pipe");
	}
	else if(complete == -2)
	{
		// Protocol error:
		if(peer != NULL)
			error("Protocol error reading from peer");
		else
			error("Protocol error reading from library endpoint");
	}
	else
	{
		// Complete message received:
		if(peer != NULL)
			serve_peer(abst, peer);
		else
			serve_endpoint(abst, mp);
		progress_in[fd] = NULL;
		delete abst;
	}
}

void swrapper::identify_fd(int fd, smidpoint **mp, speer **peer)
{			
	int found = 0;

	if(fdstate[fd] == FDLibrary)
	{
		// Library connection - set mp
		*peer = NULL;
		for(int i = 0; i < mps->count(); i++)
		{
			*mp = mps->item(i);
			if(fd == (*mp)->fd) { found = 1; break; }
		}
	}
	else if(fdstate[fd] == FDPeer)
	{
		// Peer connection - set mp and peer
		for(int i = 0; i < mps->count(); i++)
		{
			*mp = mps->item(i);
			for(int j = 0; j < (*mp)->peers->count(); j++)
			{
				*peer = (*mp)->peers->item(j);
				if(fd == (*peer)->sock) { found = 1; break; }
			}
			if(found) break;
		}
	}
	else
	{
		error("Inappropriate fdstate %d for active file descriptor %d",
				fdstate[fd], fd);
	}
	if(!found)
		error("Unknown file descriptor returned by select()");
}

void swrapper::check_dirn(int dirn, int expected, const char *fd_type)
{
	if(dirn != expected)
	{
		const char *status;
		
		if(dirn == FDInbound)
			status = "readable";
		else if(dirn == FDOutbound)
			status = "writable";
		else if(dirn == FDUnbound)
			status = "connected";
		else
			status = "unknown";
		error("Impossible: wrapper doesn't select on a %s socket "
				"becoming %s", fd_type, status);
	}
}

void swrapper::setdefaultprivs()
{

	//This is a hack function to load some default privileges that apply to all instances
	//TODO: make this nice!
	change_privileges(new privilegeparams("","rdc","",true ));
	change_privileges(new privilegeparams("","sbus","",true ));
	change_privileges(new privilegeparams("","sbus_dbfuncs","",true ));
	change_privileges(new privilegeparams("lookup_schema","","",true ));
	change_privileges(new privilegeparams("","spoke","",true));
	change_privileges(new privilegeparams("","speek","",true));

	log("Loaded (hardcoded) default privileges\n");

}

void swrapper::run()
{
	int fd;
	int direction;
	smidpoint *mp;
	speer *peer;
	
	external_master_sock = passivesock(&listen_port);
	
	canonical_address = get_local_address(external_master_sock);
	
	fdstate[external_master_sock] = FDListen;

	running(listen_port);
	warning("Component %s listening on port %d", cpt_name, listen_port);
	
	multi->add(external_master_sock, MULTI_READ, "swrapper::run");
	multi->add(bootstrap_fd, MULTI_READ, "swrapper::run");
	for(int i = 0; i < mps->count(); i++)
	{
		mp = mps->item(i);
		if(mp->builtin == 0)
			multi->add(mp->fd, MULTI_READ, "swrapper::run");
		//printf("Privilege on Endpoint %s\n",mp->name);
		//mp->acl_ep->print_permissions();
	}
	preselect_fd = new intqueue();
	preselect_direction = new intqueue();

	if (register_with_rdc)
		register_cpt(1);
	else
		setdefaultprivs(); 		//nb if we're registering with an RDC, we'll send the privileges after we've sent the register message
								//otherwise we get some synching issues.

	

		// cache->dump();


	// Select loop:
	while(1)
	{
		// Choose a file descriptor to attend to:
		if(!preselect_fd->isempty())
		{
			fd = preselect_fd->remove();
			direction = preselect_direction->remove();
		}
		else
		{
			fd = multi->poll();
			if(fd < 0)
			{
				/* No incoming network activity to deal with, so we could perform
					any background processing now (currently there is none) */
				fd = multi->wait();
			}
			if(fd < 0)
				continue;
			direction = ((multi->last_mode() == MULTI_READ) ?
					FDInbound : FDOutbound);
			if(direction == FDOutbound && (fdstate[fd] == FDMapping ||
					fdstate[fd] == FDConnecting))
			{
				direction = FDUnbound;
				// New connections appear as writeable in select()
			}
		}

		if(fd == external_master_sock)
		{
			check_dirn(direction, FDInbound, "listening");
			begin_accept();
		}
		else if(fd == bootstrap_fd)
		{
			check_dirn(direction, FDInbound, "bootstrap");
			serve_boot();
		}
		else if(fdstate[fd] == FDAccepted)
		{
			check_dirn(direction, FDInbound, "accepted");
			//printf(">>>>>>>> going to continue, fd=%d\n",fd);
			//if (fdstate[fd]!=FDDisposing)
				continue_accept(fd);
		}
		else if(fdstate[fd] == FDWelcoming)
		{
			check_dirn(direction, FDOutbound, "welcoming");
			continue_welcome(fd);
		}
		else if(fdstate[fd] == FDGreeting)
		{
			check_dirn(direction, FDOutbound, "greeting");
			continue_greeting(fd);
		}
		else if(fdstate[fd] == FDPending)
		{
			check_dirn(direction, FDInbound, "pending");
			continue_approval(fd);
		}
		else if(fdstate[fd] == FDVisiting)
		{
			check_dirn(direction, FDOutbound, "visiting");
			continue_visit(fd);
		}
		else if(fdstate[fd] == FDDisposing)
		{
			check_dirn(direction, FDInbound, "disposing");
			continue_dispose(fd);
		}
		else if(fdstate[fd] == FDMapping)
		{
			check_dirn(direction, FDUnbound, "mapping");
			continue_mapping(fd);
		}
		else if(fdstate[fd] == FDConnecting)
		{
			check_dirn(direction, FDUnbound, "connecting");
			continue_connect(fd);
		}
		else
		{
			// OK, identify midpoint or peer concerned (sets mp and maybe peer):
			identify_fd(fd, &mp, &peer);

			if(direction == FDOutbound)
				continue_write(fd, peer);
			else if(direction == FDInbound)
				continue_read(fd, mp, peer);
			else
				error("Impossible value for fd direction");
		}

	}
	close(external_master_sock);
}

void swrapper::serve_goodbye(speer *peer, sgoodbye *oob_goodbye)
{
	// Check fields match:
	if(strcmp(oob_goodbye->src_cpt, peer->cpt_name))
		error("Remote component name mismatch in serve_goodbye()");
	if(strcmp(oob_goodbye->src_ep, peer->endpoint))
		error("Remote endpoint name mismatch in serve_goodbye()");
	if(strcmp(oob_goodbye->tgt_cpt, cpt_name))
		error("Local component name mismatch in serve_goodbye()");
	if(strcmp(oob_goodbye->tgt_ep, peer->owner->name))
		error("Local endpoint name mismatch in serve_goodbye()");

	// Process goodbye:
	log("Peer %s, endpoint %s closed connection with Goodbye",
			peer->cpt_name, peer->endpoint);
	departure(peer, 0);
}

void swrapper::serve_resub(speer *peer, sresub *oob_resub)
{
	// Check fields match:
	if(strcmp(oob_resub->src_cpt, peer->cpt_name))
		error("Remote component name mismatch in serve_resub()");
	if(strcmp(oob_resub->src_ep, peer->endpoint))
		error("Remote endpoint name mismatch in serve_resub()");
	if(strcmp(oob_resub->tgt_cpt, cpt_name))
		error("Local component name mismatch in serve_resub()");
	if(strcmp(oob_resub->tgt_ep, peer->owner->name))
		error("Local endpoint name mismatch in serve_resub()");


	// Change subscription/topic:
	if(peer->subs != NULL)
		//TODO: This used to be delete[], but was crashing. Not sure if the intention was a single subscription per peer--- it crashes if not, so lets assume yes....
		delete peer->subs;
	if(peer->topic != NULL)
		delete peer->topic;
	if (oob_resub->subscription != NULL)
		peer->subs = new subscription(oob_resub->subscription);
	else
		peer->subs = NULL;
	peer->topic = sdup(oob_resub->topic);
	log("Resubscribed peer '%s:%s:%s (%s)' - filter '%s'",peer->cpt_name,peer->instance,peer->endpoint, peer->address, oob_resub->subscription);

}

void swrapper::serve_divert(speer *peer, sdivert *oob_divert)
{
	smidpoint *mp;
	
	mp = peer->owner;
	// Check fields match:
	if(strcmp(oob_divert->src_cpt, peer->cpt_name))
		error("Remote component name mismatch in serve_divert()");
	if(strcmp(oob_divert->src_ep, peer->endpoint))
		error("Remote endpoint name mismatch in serve_divert()");
	if(strcmp(oob_divert->tgt_cpt, cpt_name))
		error("Local component name mismatch in serve_divert()");
	if(strcmp(oob_divert->tgt_ep, mp->name))
		error("Local endpoint name mismatch in serve_divert()");

	// Process divert request; first remove mapping:
	departure(peer, 0);
	
	// Begin the process of remapping to the new component/endpoint:
	map(mp, oob_divert->new_cpt, oob_divert->new_ep, -1);
}

void swrapper::serve_peer(AbstractMessage *abst, speer *peer)
{
	scomm *msg;
	int ret;

	msg = new scomm();	
	ret = msg->reveal(abst);
	if(ret == -1)
		error("Protocol error: wrong message type received from peer");
	serve_peer(msg, peer);	
}

void swrapper::serve_peer(scomm *msg, speer *peer)
{
	// Read scomm
	smidpoint *mp;
	snode *sn;
	Schema *schema;
	const char *err;
	HashCode *expected_hc;

	mp = peer->owner;

	// Check for OOB:
	if(msg->type == MessageGoodbye)
	{
		log("Wrapper received remote Goodbye message on endpoint %s",
				peer->owner->name);
		serve_goodbye(peer, msg->oob_goodbye);
		delete msg;
		return;
	}
	else if(msg->type == MessageDivert)
	{
		log("Wrapper received remote Divert message on endpoint %s",
				peer->owner->name);
		serve_divert(peer, msg->oob_divert);
		delete msg;
		return;
	}
	else if(msg->type == MessageResubscribe)
	{
		serve_resub(peer, msg->oob_resub);
		delete msg;
		return;
	}

	/* Check if hash code matches a schema in the cache;
		if not initiate a schema lookup from this peer and put this
		(scomm, peer) pair on hold, ready to re-call serve_peer() later: */
	
	schema = cache->lookup(msg->hc);
	if(schema == NULL)
	{
		// Ready to pause this action, saved state = (scomm, peer):
		msg->peer_uid = peer->uid;
		mp->waiting++;

		// Initiate schema lookup:
		snode *sn;
		int ok;
		const char *s;
		s = msg->hc->tostring();
		sn = pack(s, "hashcode");
		delete[] s;
		ok = begin_visit(VisitLookupSchema, peer->address, peer->cpt_name,
				"lookup_schema", lookup_schema_mp, sn, NULL, NULL, msg);
		/* Note: finalise_server_visit() will restart serve_peer() later,
			after adding the schema to the cache */
		if(ok < 0)
		{
			/* Couldn't make contact back to component, hence lookup
				isn't going ahead. Without the schema, we can't process
				the original message. */
			warning("Warning: schema lookup on component '%s' failed",
					peer->address);
			warning("Cannot process incoming message without relevant schema");
			delete msg;
		}
		return;
	}
			

	// Check fields match:
	if(strcmp(msg->source, peer->cpt_name))
		error("Remote component name mismatch in serve_peer()");
	if(strcmp(msg->src_endpoint, peer->endpoint))
		error("Remote endpoint name mismatch in serve_peer()");
	if(strcmp(msg->target, cpt_name))
		error("Local component name mismatch in serve_peer()");
	if(strcmp(msg->tgt_endpoint, mp->name))
		error("Local endpoint name mismatch in serve_peer()");
	
	// Check hash code:
	if(msg->type == MessageClient) // This is a reply
		expected_hc = mp->reply_hc;
	else
		expected_hc = mp->msg_hc;
	
	// If we're doing flexible matching, don't check types.
	if (mp->flexible_matching)
	{
		log("Received a flexible matching message");
	}	
	else if(!expected_hc->ispolymorphic() && !msg->hc->equals(expected_hc))
	{
		if((msg->type == MessageClient && peer->reply_poly) ||
			(msg->type != MessageClient && peer->msg_poly))
		{
			// Not an error - just print a warning and discard message:
			warning("Warning: dropping message of wrong type from polymorphic"
					" peer");
			delete msg;
			return;
		}
		else
			error("Unexpected message hash code in serve_peer()");
	}
	// schema already set, so don't need mp->msg_schema or mp->reply_schema


	// Parse data:
	sn = unmarshall(msg->data, msg->length, schema, &err);
	if(sn == NULL)
		error("Error unmarshalling data from the network:\n%s", err);


	// Update counter:
	mp->processed++;

	/*** HACK: for unauthorised disposable conns, we need to unmarshall the stuff etc for a nice disconnect..
	 * TODO: fix this properly!
	 */
	if (msg->terminate_disposable)
	{
		departure(peer, 0);
		return;
	}
	
	if(mp->builtin)
	{
		serve_builtin(peer, sn, msg->seq);
		delete msg;
		return;
	}
	
	switch(msg->type)
	{
		case MessageSink:
			log("Wrapper received remote Sink message on endpoint %s",
					peer->owner->name);
			if(mp->type != EndpointSink)
				error("MessageSink sent to a non-sink endpoint");
			peer->sink(sn, msg->hc, msg->topic);
			if(peer->disposable)
				departure(peer, 0);
			break;
		case MessageServer:
			log("Wrapper received remote Server message on endpoint %s",
					peer->owner->name);
			if(mp->type != EndpointServer)
				error("MessageServer sent to a non-server endpoint");
			peer->serve(sn, msg->seq, msg->hc);
			break;
		case MessageClient:
			log("Wrapper received remote Client message on endpoint %s",
					peer->owner->name);
			if(mp->type != EndpointClient)
				error("MessageClient sent to a non-client endpoint");
			peer->client(sn, msg->seq, msg->hc);
			break;
		default:
			error("Unknown message type in serve_peer()");
	}

	// Don't have to delete sn - it will be consumed by the method call above	
	delete msg;
}

snode *swrapper::pack_metadata()
{
	snode *sn, *sn_keywords, *sn_eps, *sn_ep;
	smidpoint *mp;
	char *s;
	
	sn_keywords = mklist("keywords");
	for(int i = 0; i < admin->keywords->count(); i++)
		sn_keywords->append(pack(admin->keywords->item(i), "keyword"));
	
	sn_eps = mklist("endpoints");
	for(int i = 0; i < mps->count(); i++)
	{
		mp = mps->item(i);
		if(mp->builtin)
			continue;
		//if(!mp->acl_ep->check_authorised())
		sn_ep = mklist("endpoint");
		sn_ep->append(pack(mp->name, "name"));
		sn_ep->append(pack(endpoint_type[mp->type], "type"));
		
		s = mp->msg_schema->orig_string();
		sn_ep->append(pack(s, "message"));
		delete[] s;
		
		s = mp->reply_schema->orig_string();
		sn_ep->append(pack(s, "response"));
		delete[] s;
		
		sn_eps->append(sn_ep);
	}
	
	sn = mklist("component-metadata");
	sn->append(pack(cpt_name, "name"));
	sn->append(pack(admin->description, "description"));
	sn->append(sn_keywords);
	sn->append(pack(admin->designer, "designer"));
	sn->append(sn_eps);
	
	return sn;
}

snode *swrapper::pack_status()
{
	snode *sn, *sn_load, *sn_fresh;
	snode *sn_eps, *sn_ep, *sn_peers, *sn_peer;
	smidpoint *mp;
	speer *peer;

	sn_load = pack(pack(0, "cpu"), pack(0, "buffers"), "load");
	sn_fresh = pack(SNull, "freshness");
	
	sn_eps = mklist("endpoints");
	for(int i = 0; i < mps->count(); i++)
	{
		mp = mps->item(i);

		sn_peers = mklist("peers");
		for(int j = 0; j < mp->peers->count(); j++)
		{
			peer = mp->peers->item(j);
			if(peer->disposable)
				continue;

			sn_peer = mklist("peer");
			sn_peer->append(pack(peer->cpt_name, "cpt_name"));
			sn_peer->append(pack(peer->instance, "instance"));
			// Should really have an ep_id field here - XXX
			sn_peer->append(pack(peer->endpoint, "endpoint"));
			sn_peer->append(pack(peer->address, "address"));
			if(peer->subs == NULL)
				sn_peer->append(pack(SNull, "subscription"));
			else
			{
				char *s = peer->subs->tostring();
				sn_peer->append(pack(s, "subscription"));
				delete[] s;
			}
			sn_peer->append(pack(peer->topic, "topic"));
			sn_peer->append(pack(0, "lastseq"));
			sn_peer->append(pack(0, "latency"));

			sn_peers->append(sn_peer);
		}

		sn_ep = mklist("endpoint");
		sn_ep->append(pack(mp->name, "name"));
		sn_ep->append(pack(mp->processed, "processed"));
		sn_ep->append(pack(mp->dropped, "dropped"));
		sn_ep->append(pack(mp->subs, "subscription"));
		sn_ep->append(pack(mp->topic, "topic"));
		sn_ep->append(sn_peers);

		sn_eps->append(sn_ep);
	}
	
	sn = mklist("component-state");
	sn->append(pack(canonical_address, "address"));
	sn->append(pack(instance_name, "instance"));
	sn->append(pack(admin->creator, "creator"));
	sn->append(pack(cmdline, "cmdline"));
	sn->append(sn_load);
	sn->append(sn_eps);
	sn->append(sn_fresh);
	
	return sn;
}

snode *swrapper::pack_privileges()
{
	snode *sn, *sn_auths, *sn_rule,  *sn_eps, *sn_ep;
	smidpoint *mp;
	char *s;

	sn_eps = mklist("endpoints");
	for(int i = 0; i < mps->count(); i++)
	{
		mp = mps->item(i);
		sn_ep = mklist("endpoint");
		sn_ep->append(pack(mp->name, "ep_name"));
		sn_auths = mklist("authorisations");
		for (int j = 0; j < mp->acl_ep->count(); j++)
		{
			sn_rule = mklist("privilege");
			sn_rule->append(pack(mp->acl_ep->item(j)->principal_cpt, "principal"));
			sn_rule->append(pack(mp->acl_ep->item(j)->principal_inst, "instance"));
			sn_auths->append(sn_rule);
		}
		sn_ep->append(sn_auths);
		sn_eps->append(sn_ep);
	}


	sn = mklist("cpt_privileges");
	sn->append(pack(cpt_name, "name"));
	sn->append(pack(instance_name, "instance"));
	sn->append(pack(canonical_address, "address"));
	sn->append(sn_eps);
//	printf("Built up the permissions node:\n%s\n", sn->toxml(1));
	return sn;
}


void swrapper::serve_builtin(speer *peer, snode *sn, int seq)
{
	smidpoint *mp;
	
	mp = peer->owner;

	if(mp->type == EndpointServer)
		serve_rpc_builtin(peer, sn, seq);
	else if(mp->type == EndpointSink)
		serve_sink_builtin(peer->owner->name, sn);
	else
		error("Only built-in endpoints of type server and sink exist");
}

void swrapper::serve_sink_builtin(const char *fn_endpoint, snode *sn)
{
	smidpoint *mp = NULL;
	const char *op_endpoint, *cert;
	int index;
	
	log("Wrapper received remote Sink message for built-in"
			" endpoint %s", fn_endpoint);
	if(sn->exists("certificate"))
		cert = sn->extract_txt("certificate");
	if(sn->exists("endpoint"))
	{
		op_endpoint = sn->extract_txt("endpoint");
		for(index = mps->count() - 1; index >= 0; index--)
		{
			mp = mps->item(index);
			if(!strcmp(mp->name, op_endpoint))
				break;
		}
		if(index < 0)
		{
			warning("Other component requested a built-in operation on "
					"non-existant endpoint '%s'", op_endpoint);
			return;
		}
	}
	
	if(!strcmp(fn_endpoint, "map"))
	{
		const char *peer_address, *peer_endpoint;
		
		peer_address = sn->extract_txt("peer_address");
		peer_endpoint = sn->extract_txt("peer_endpoint");
		map(mp, peer_address, peer_endpoint, -1);
	}
	else if(!strcmp(fn_endpoint, "unmap"))
	{
		const char *peer_address, *peer_endpoint;
		if (sn->exists("peer_address"))
			peer_address = sn->extract_txt("peer_address");
		else
			peer_address = NULL;
		if (sn->exists("peer_endpoint"))
			peer_endpoint = sn->extract_txt("peer_endpoint");
		else
			peer_endpoint = NULL;
		unmap(mp, peer_address, peer_endpoint, -1);
	}
	else if(!strcmp(fn_endpoint, "divert"))
	{
		
		//should prob be its own function
		int address_type;
		mapparams *params;
		const char *peer_address;
		params = new mapparams();
		params->mp = mp;
		params->operation = RSDivert;
		params->newaddr = sn->extract_txt("new_address");
		params->newendpt = sn->extract_txt("new_endpoint");
		if (sn->exists("peer_endpoint"))
			params->endpoint = sn->extract_txt("peer_endpoint");
		if (sn->exists("peer_address"))
			peer_address = sn->extract_txt("peer_address");
		else
			peer_address = address_wildcard; //hack placeholder replacing NULL to mean all...
		address_type  = MapConstraints::is_constraint(peer_address);
		if(address_type == -1)
		{
			delete params;
			error("Problem with the peer address '%s'\n",peer_address);
		}
		else if(address_type == 1)
			resolve_address_local(mp, peer_address, params);
		else if(address_type == 0)
			params->local_possibilities->add(peer_address);
		do_divert(params);

	}
	else if(!strcmp(fn_endpoint, "subscribe"))
	{
		const char *endpoint, *peer, *subs, *topic;
		
		endpoint = sn->extract_txt("endpoint");
		peer = sn->extract_txt("peer");
		subs = sn->extract_txt("subscription");
		topic = sn->extract_txt("topic");
		
		subscribe(mp, subs, topic, peer);
	}
	else if(!strcmp(fn_endpoint, "set_log_level"))
	{
		log_level = sn->extract_int("log");
		echo_level = sn->extract_int("echo");
	}
	else if(!strcmp(fn_endpoint, "register_rdc"))
	{		
		// get the address of this new rdc and whether it is to be added or removed.
		const char *address = sn->extract_txt("rdc_address");
		int arrived = sn->extract_flg("arrived");
		
		// Usually we'd send a message to register_rdc when connecting to a new network.
		// Therefore, it stands to reason that our IP could be different, so we need to get the new one.
		canonical_address = get_local_address(external_master_sock);
		
		if (rdc_update_autoconnect)
		{
			// register/deregister with the rdc.
			handle_new_rdc(arrived, address);
		}
		
		if (rdc_update_notify)
		{
			HashCode *hc;
			hc = new HashCode();
			hc->fromschema("@event { txt rdc_address flg arrived }");
			
			snode *sn;
			sn = pack(pack(address, "rdc_address"), pack_bool(arrived, "arrived"));
			
			smessage *msg;
	
			msg = new smessage();
			msg->type = MessageRcv;
			msg->source_cpt = NULL;
			msg->source_inst = NULL;
			msg->source_ep = NULL;
			msg->topic = NULL;
			msg->source_ep_id = 0;
			msg->seq = 0;
			msg->hc = new HashCode(hc);
			msg->tree = sn; // Consumes sn
			
			rdc_update_mp->deliver_local(msg);
		}
	}
	else if(!strcmp(fn_endpoint, "terminate"))
	{
		warning("Abort: Wrapper terminating in response to remote request");
		if (register_with_rdc)
			register_cpt(0);
		
		exit(0);
	}
	else if(!strcmp(fn_endpoint,"access_control"))
	{
//		printf("Processing an access control request %s\n",sn->toxml(1));
		log("Processing an access control request\n");
		privilegeparams *priv = new privilegeparams();
		priv->p_cpt = sn->extract_txt("principal_cpt");
		priv->p_inst = sn->extract_txt("principal_inst");
		if (sn->exists("target_ept"))
			priv->target_endpt = sn->extract_txt("target_ept");
		priv->addpriv = sn->extract_flg("authorised");
		change_privileges(priv);
	}
	else
		error("Unknown built-in endpoint sink name");
}



void speer::deliver_remote(scomm *msg, int disrupt)
{
	AbstractMessage *abst;
	AbstractMessageQueue *q;
	
	abst = msg->wrap(sock);
	q = wrap->progress_out[sock];
	if(!q->isempty()) // Already a message being sent
		q->add(abst);
	else
	{
		if(disrupt)
		{
			// Test partial writes
			abst->partial_advance();
			sleep(15);
		}
		wrap->preselect_fd->add(sock);
		wrap->preselect_direction->add(FDOutbound);
		wrap->multi->add(sock, MULTI_WRITE, "speer::deliver_remote");
		q->add(abst);
	}
}
	
void swrapper::serve_rpc_builtin(speer *peer, snode *sn, int seq)
{
	char *endpoint;
	snode *snrep;
	scomm *msg;
	const char *err;
	unsigned char *buf;
	int length;
	smidpoint *mp;
	
	mp = peer->owner;
	endpoint = mp->name;
	log("Wrapper received remote Server message for built-in"
			" endpoint %s", endpoint);
	// Form reply in snrep:
	if(!strcmp(endpoint, "get_metadata"))
		snrep = pack_metadata();
	else if(!strcmp(endpoint, "get_status"))
		snrep = pack_status();
	else if(!strcmp(endpoint, "lookup_schema"))
	{
		const char *hsh;
		Schema *sch;
		HashCode *hc;
		char *s;
		hc = new HashCode();
		hsh = sn->extract_txt();
		hc->fromstring(hsh);
		sch = cache->lookup(hc);
		if(sch == NULL)
			snrep = pack("!", "schema");
		else
		{
			s = sch->orig_string();
			snrep = pack(s, "schema");
			delete[] s;
		}
		delete hc;
	}
	else if(!strcmp(endpoint, "dump_privileges"))
		snrep = pack_privileges();
	else
		error("Unknown built-in endpoint service name");	


	log("Replying to %s's request on ept '%s'\n", peer->cpt_name, mp->name );
	msg = new scomm();
	msg->source = sdup(*(mp->cpt_name));
	msg->src_endpoint = sdup(mp->name);
	msg->target = sdup(peer->cpt_name);
	msg->tgt_endpoint = sdup(peer->endpoint);
	msg->type = MessageClient;
	msg->seq = seq;
	msg->hc = new HashCode(mp->reply_hc);

	buf = marshall(snrep, mp->reply_schema, &length, &err);
	if(buf == NULL)
		error("Built-in reply message does not conform with schema:\n%s", err);
	msg->data = buf;
	msg->length = length;
	delete snrep;

	peer->deliver_remote(msg);
	delete msg; // msg will delete buf for us
}

void swrapper::add_builtin_endpoints()
{
	get_status_mp = add_builtin("get_status", EndpointServer, "000000000000",
			"253BAC1C33C7");
	add_builtin("get_metadata", EndpointServer, "000000000000",
			"6306677BFE43");
	
	add_builtin("map", EndpointSink, "F46B9113DB2D");
	add_builtin("unmap", EndpointSink, "FCEDAD0B6FE1");
	add_builtin("divert", EndpointSink, "C648D3D07AE8");
	add_builtin("subscribe", EndpointSink, "72904AC06922");
	add_builtin("register_rdc", EndpointSink, "13ACF49714C5");

	register_mp = add_builtin("register", EndpointSource, "B3572388E4A4");
	lookup_cpt_mp = add_builtin("lookup_cpt", EndpointClient, "18D70E4219C8",
			"F96D2B7A73C1");
		
	add_builtin("lookup_schema", EndpointServer, "897D496ADE90",
			"D39E44946A6C");
	
	lookup_schema_mp = add_builtin("lookup_schema", EndpointClient,
			"897D496ADE90", "D39E44946A6C");
	
	add_builtin("set_log_level", EndpointSink, "D8A30E59C04A");
	add_builtin("terminate", EndpointSink, "000000000000");
	
	lost_mp = add_builtin("lost", EndpointSource, "B3572388E4A4");
	
	map_policy_mp = add_builtin("map_policy", EndpointSource, "857FC4B7506D");

	//for AC
	add_builtin("access_control", EndpointSink, "470551F178B5");
	dump_privilege_mp = add_builtin("dump_privileges", EndpointServer, "000000000000", "720B9A3C44F3");

	rdcacl_mp = add_builtin("rdc_privs", EndpointSource, "6AF2ED96750B"); //communicate privs to the ACL...
}

smidpoint *swrapper::add_builtin(const char *endpoint, EndpointType type,
		const char *msg_hash, const char *reply_hash)
{
	smidpoint *mp;
	
	mp = new smidpoint();
	mp->builtin = 1;
	mp->cpt_name = &cpt_name;
	mp->name = sdup(endpoint);
	mp->ep_id = mps->count();
	mp->type = type;
	mp->msg_hc = new HashCode();
	mp->msg_hc->fromstring(msg_hash);
	mp->reply_hc = new HashCode();
	if(reply_hash == NULL)
		mp->reply_hc->frommeta(SCHEMA_NA);
	else
		mp->reply_hc->fromstring(reply_hash);
	mps->add(mp);
	return mp;
}

smidpoint *swrapper::add_endpoint(saddendpoint *add)
{
	smidpoint *mp;
	
	mp = new smidpoint();
	mp->builtin = 0;
	mp->cpt_name = &cpt_name;
	mp->name = sdup(add->endpoint);
	mp->ep_id = mps->count();
	mp->type = add->type;
	mp->msg_hc = new HashCode(add->msg_hc);
	mp->reply_hc = new HashCode(add->reply_hc);
	mp->flexible_matching = add->flexible_matching;
	mp->acl_ep = new spermissionvector();
	
	//TODO:Load ACL defaults...

	// Connect back to library:
	mp->fd = activesock(callback_address);
	if(mp->fd < 0)
	{
		error("Can't reach application on callback address '%s'",
				callback_address);
	}
	if(nonblocking)
		sock_noblock(mp->fd);
	fdstate[mp->fd] = FDLibrary;
	
	mps->add(mp);
	return mp;
}

void swrapper::subscribe(smidpoint *mp, const char *subs,
		const char *topic, const char *peer_address)
{
	int address_type;
	const char *cpt, *inst, *addr;
	speer *p;
	snode *constraints;

	int all = 0;
	if(mp->type != EndpointSink)
		error("Tried to set subscription on a non-sink endpoint");
	if(peer_address == NULL)
	{
		// Set default subscription/topic for new maps
		
		if(mp->subs != NULL)
			delete[] mp->subs;
		if(mp->topic != NULL)
			delete[] mp->topic;
		mp->subs = sdup(subs);
		mp->topic = sdup(topic);
		return;
	}


	//if(peer_address[0] == '*' && peer_address[1] == '\0')
	if (!strcmp(peer_address, "") || !strcmp(peer_address, "*"))
		all = 1;
	else
	{
		cpt = sdup("");
		inst = sdup("");
		addr=sdup("");
		address_type  = MapConstraints::is_constraint(peer_address);
		if(address_type == -1)
			error("Problem with the peer address '%s' in subscription\n",peer_address);
		else if(address_type == 1)
		{
			//if the peer was referred to by name
			MapConstraints *mapcon;
			mapcon = new MapConstraints(peer_address);

			//TODO: Add in other constraints (certificates, pub keys, etc).
			if (mp->flexible_matching)
				constraints = mapcon->pack(mp->msg_schema->hashes);
			else
				constraints = mapcon->pack();
			//printf("the constraints for this filter operation are %s\n",constraints->toxml(1));
			if(constraints->exists("cpt-name"))
				cpt = constraints->extract_txt("cpt-name");
			if(constraints->exists("instance-name"))
				inst = constraints->extract_txt("instance-name");
		}
		else if(address_type == 0)
			addr = sdup(peer_address);
	}

	for(int i = 0; i < mp->peers->count(); i++)
	{
		p = mp->peers->item(i);
		if(all)
			p->resubscribe(subs, topic);
		else
		{
			if ( (!strcmp("",cpt) || !strcmp(p->cpt_name,cpt)  )
				&& (!strcmp("",inst) || !strcmp(p->instance,inst) )
						&& (!strcmp("",addr) || !strcmp(p->address,addr) ) )
							p->resubscribe(subs, topic);
		}
	}
}

Schema *swrapper::declare_schema(const char *schema, int file_lookup)
{
	// Add to schema cache
	Schema *sch, *cache_copy;
	const char *err;
	int already_known;

	if(file_lookup)
	{
		// schema contains a file name rather than the actual schema text:
		char *located;
		
		located = path_lookup(schema);
		sch = Schema::load(located, &err);
		if(sch == NULL)
			error("Error reading schema from %s:\n%s", located, err);
		delete[] located;
	}
	else
	{
		// The schema string contains the schema itself:
		sch = Schema::create(schema, &err);
		if(sch == NULL)
		{
			warning("Declared schema is invalid!\n%s", err);
			return NULL;
		}
	}
	cache_copy = cache->add(sch, &already_known);
	if(!already_known)
	{
		log("Declared new schema (added to cache):");
		sch->dump_tree(0, 1);
	}
	delete sch;
	return cache_copy;
}

snode *smidpoint::pack_interface(int invert)
{
	snode *sn, *subn;
	char *s, *t;
	int ty;
	
	if(invert)
	{
		switch(type)
		{
			case EndpointServer: ty = EndpointClient; break;
			case EndpointClient: ty = EndpointServer; break;
			case EndpointSource: ty = EndpointSink; break;
			case EndpointSink:   ty = EndpointSource; break;
			default: error("Impossible switch error in pack_interface");
		}
	}
	else
		ty = type;
	
	sn = mklist("interface");
	subn = mklist("endpoint");
	subn->append(pack(name, "name"));
	subn->append(pack(endpoint_type[ty], "type"));
	s = msg_hc->tostring();
	t = reply_hc->tostring();
	subn->append(pack(s, "msg-hash"));
	subn->append(pack(t, "reply-hash"));
	delete[] s;
	delete[] t;
	sn->append(subn);
	return sn;
}

void swrapper::abort_visit(AbstractMessage *abst)
{
	/* A visitor message has failed; take any required actions depending
		on its purpose: */
	
	if(abst->purpose == VisitLookupSchema)
	{
		// This means unrecognised != NULL
		
		// Failed to contact component for lookup_schema:
		warning("Warning: schema lookup on component '%s' failed",
				abst->address);
		warning("Cannot process incoming message without relevant schema");
		delete abst->unrecognised;
	}
	else if(abst->purpose == VisitRegister)
	{
		// This means reg_params != NULL
		
		registerparams *reg_params = abst->reg_params;
		int arrive = reg_params->arrive;
		
		reg_params->failed++;
		if(reg_params->failed + reg_params->succeeded == reg_params->count)
		{
			// Tried them all now
			if(reg_params->failed == reg_params->count && arrive)
					warning("Warning: no RDCs available to register with");
			delete reg_params;
		}
	}
	else if(abst->purpose == VisitResolveConstraints)
	{
		// This means map_params != NULL

		/* No results to add to the list of possibilities; just continue
			with the next RDC (or go onto mapping stage if none left): */
		log("Note: could not consult RDC at %s", abst->address);
		abst->map_params->remaining_rdcs--;		
		continue_resolve(abst->map_params);
	}	
	else if(abst->purpose == VisitLost)
	{
		/* Do nothing: lost reports are advisory only, to speed up
			the response time of the RDC (and may well be sent by
			multiple components anyway). Thus if a lost event is,
			well, lost, no harm is done. */
	}
	if(abst->purpose == VisitUpdatePrivilege)
	{
		// This means unrecognised != NULL

		// Failed to contact component for lookup_schema:
		warning("Warning: Updating privileges failed for '%s' failed",abst->address);
		delete abst->unrecognised;
	}
	else
		error("Unknown reason for visit %d in abort_visit()", abst->purpose);
}

void swrapper::continue_connect(int fd)
{
	AbstractMessage *abst;
	int ret, so_error;
	socklen_t len;

	abst = progress_con[fd];
	progress_con[fd] = NULL;
	
	/* Check if connection was successful */
	
	so_error = 666;
	len = sizeof(so_error);
	ret = getsockopt(fd, SOL_SOCKET, SO_ERROR, &so_error, &len);
	if(ret < 0)
		error("Cannot getsockopt()");
	if(so_error != 0)
	{
		/* Take required action for failure to connect, depending on
			what the connection was for: */
		log("Connection to %s failed - %s",
				abst->address == NULL ? "unknown" : abst->address,
				strerror(so_error));
		abort_visit(abst);
		
		/* Cleanup: */
		multi->remove(fd, MULTI_WRITE);
		delete abst;
		fdstate[fd] = FDUnused;
		return;
	}
	/*
		SO_ERROR at SOL_SOCKET level:
		0 = success, other = errno as for blocking connect()
	*/
		
	/* Preselect to send visitor message */

	/* printf("DBG> so_error = %d; connection to %s successful\n",
			so_error, abst->address == NULL ? "unknown" : abst->address); */
	preselect_fd->add(fd);
	preselect_direction->add(FDOutbound);
	// Don't need multi->add(fd, MULTI_WRITE), as already done for connect
	AbstractMessageQueue *q = progress_out[fd];
	sassert(q->isempty(), "progress_out queue not empty");
	q->add(abst);
	fdstate[fd] = FDVisiting;
}

int swrapper::begin_visit(VisitPurpose purpose, const char *addr,
		const char *cpt, const char *ep, smidpoint *mp, snode *sn,
		mapparams *map_params, registerparams *reg_params,
		scomm *unrecognised)
{
	// Note: mp is used just for hash codes - it doesn't have to be mapped
	// map_params is optional, may be NULL
	// unrecognised is optional, may be NULL
	// Returns a fd, or -1 if can't create a connection

	const char *err;
	unsigned char *buf;
	int length;
	svisitor *visitor;
	AbstractMessage *abst;
	int fd;

	/* Create visitor message */
	//printf("going to marshall schema = %s\n",mp->msg_schema->canonical_string());
	buf = marshall(sn, mp->msg_schema, &length, &err);
	//printf("buf worked\n");
	if(buf == NULL)
		error("Visitor message does not conform with schema:\n%s", err);
	visitor = new svisitor();
	visitor->src_cpt = sdup(cpt_name);
	visitor->src_instance = sdup(instance_name);
	visitor->dispose_id = next_visit++;
	visitor->tgt_cpt = sdup(cpt);
	visitor->required_endpoint = sdup(ep);
	if(mp->type == EndpointSource)
		visitor->ep_type = EndpointSink;
	else if(mp->type == EndpointClient)
		visitor->ep_type = EndpointServer;
	else
	{
		error("Tried to visit wrong type of endpoint '%s'",
				endpoint_type[mp->type]);
	}
	visitor->msg_hc = new HashCode(mp->msg_hc);
	visitor->reply_hc = new HashCode(mp->reply_hc);
	visitor->length = length;
	visitor->data = buf;

	/* Open connection */

	if(nonblockingconnect)
	{

		fd = activesock(addr, ACTIVE_SOCK_SILENT | ACTIVE_SOCK_NONBLOCK);
		if(fd == -1)
			return -1;


		/* Create AbstractMessage */
		abst = visitor->wrap(fd);
		delete visitor; // Deleting visitor will delete buf for us
		abst->purpose = purpose;
		if(mp->type == EndpointSource)
				abst->reply_schema = NULL;
		else
			abst->reply_schema = mp->reply_schema;
		abst->map_params = map_params;
		abst->reg_params = reg_params;
		abst->unrecognised = unrecognised;
		abst->address = sdup(addr);
		mp->processed++;
		preselect_fd->add(fd);
		preselect_direction->add(FDUnbound);
		// Connected sockets become writeable:
		multi->add(fd, MULTI_WRITE, "swrapper::begin_visit");
		progress_con[fd] = abst;
		fdstate[fd] = FDConnecting;

		return fd; // OK so far...
	}
	
	fd = activesock(addr, ACTIVE_SOCK_SILENT);
	if(fd == -1)
		return -1;
	if(nonblocking)
		sock_noblock(fd);
	
	/* Create AbstractMessage */


	abst = visitor->wrap(fd);
	delete visitor; // Deleting visitor will delete buf for us
	abst->purpose = purpose;
	if(mp->type == EndpointSource)
		abst->reply_schema = NULL;
	else
		abst->reply_schema = mp->reply_schema;
	abst->map_params = map_params;
	abst->reg_params = reg_params;
	abst->unrecognised = unrecognised;
	abst->address = sdup(addr);
	mp->processed++;
	
	/* Preselect to send visitor message */
	
	preselect_fd->add(fd);
	preselect_direction->add(FDOutbound);
	multi->add(fd, MULTI_WRITE, "swrapper::begin_visit");
	AbstractMessageQueue *q = progress_out[fd];
	sassert(q->isempty(), "progress_out queue not empty");
	q->add(abst);
	fdstate[fd] = FDVisiting;

	return fd;
}

void swrapper::finalise_sink_visit(AbstractMessage *abst)
{
	// Do something based on reason for visit...		
	
	if(abst->purpose == VisitRegister)
	{
		// This means reg_params != NULL
		registerparams *reg_params = abst->reg_params;
		int arrive = reg_params->arrive;
		
		reg_params->succeeded++;
		if(arrive)
			log("Registered with RDC at %s\n", abst->address);
		else
			log("Deregistered with RDC at %s\n", abst->address);
		if(reg_params->succeeded + reg_params->failed == reg_params->count)
		{
			// All done:
			delete reg_params;

			if(arrive)
				setdefaultprivs(); //now we've sent the registered msgs, let's load the default privs.
		}
	}
	else if(abst->purpose == VisitLost)
	{
		/* No special action needs to be taken */
	}
	else if(abst->purpose == VisitUpdatePrivilege)
	{
		/* No special action needs to be taken */
		//warning("Finalising the visit for privilege update\n");
	}
	else
	{
		error("Unknown reason for visit %d in finalise_sink_visit()",
				abst->purpose);
	}
}

void swrapper::finalise_server_visit(AbstractMessage *abst)
{
	snode *sn_results;
	int ret;
	scomm *reply;
	const char *err;
	Schema *reply_schema;

	reply_schema = abst->reply_schema;
	reply = new scomm();
	ret = reply->reveal(abst);
	if(ret == -1 || reply->type != MessageClient)
		error("Protocol error: wrong reply message type to visit");

	// We omit check for source and destination cpt/ep field match here
	// We omit check for hash code match here

	sn_results = unmarshall(reply->data, reply->length, reply_schema, &err);
	if(sn_results == NULL)
		error("Error unmarshalling reply from visit:\n%s", err);
	delete reply;

	// Do something with sn_results, based on reason for visit...	
	if(abst->purpose == VisitLookupSchema)
	{
		// This means unrecognised != NULL
		scomm *unrecognised = abst->unrecognised;

		const char *err;
		Schema *sch;
		speer *peer;
		smidpoint *mp;
		int found = 0;

		for(int i = 0; i < mps->count(); i++)
		{
			mp = mps->item(i);
			for(int j = 0; j < mp->peers->count(); j++)
			{
				peer = mp->peers->item(j);
				if(peer->uid == unrecognised->peer_uid)
				{
					found = 1;
					break;
				}
			}
			if(found) break;
		}
		if(found == 0)
		{
			/* The peer which sent the unrecognised message seems to have
				departed, so no need to continue processing it */
			delete sn_results;
			delete abst;
			return;
		}

		// Add to schema cache:
		sch = Schema::create(sn_results->extract_txt(), &err);
		if(sch == NULL)
			error("Schema learned from peer component %s via lookup_schema "
					"is invalid!\n%s", peer->cpt_name, err);
		cache->add(sch);
		log("Learned new schema (added to cache):");
		sch->dump_tree(0, 1);

		// Restart processing of message paused due to unrecognised hash code (if we looked up on receipt of message):
		if (mp->waiting > 0)
		{
			delete sch;
			mp->waiting--;
			serve_peer(unrecognised, peer);
		}
		else
		{
			// Got schema, construct the lookup table.
			construct_peer_lookup(mp, peer, sch);
			delete sch;
		}
	}
	else if(abst->purpose == VisitResolveConstraints)
	{
		// This means map_params != NULL
		mapparams *map_params = abst->map_params;

		// Add results to the list:
		for(int i = 0; i < sn_results->count(); i++)
		{	
			map_params->possibilities->add_noduplicates(sn_results->extract_item(i)->extract_txt("address"));
		}
		map_params->remaining_rdcs--;
		continue_resolve(map_params);
	}
	else
	{
		error("Unknown reason for visit %d in finalise_server_visit()",
				abst->purpose);
	}
	delete abst;
	delete sn_results;
}

void swrapper::construct_peer_lookup(smidpoint *mp, speer *peer, Schema *peer_schema)
{
	/**
	  * Construct lookup table based on the map constraint string we used.
	  * Everything covered by constraint string, i.e. any fields and their descendants will be added to the lookup.
	  */
	if (peer->map_constraint != NULL)
	{
		MapConstraints *mapcon = new MapConstraints(peer->map_constraint);
		const char *schema_constraint = mapcon->pack(mp->msg_schema->hashes)->extract_txt("schema");
		snode *constraints = snode::import(schema_constraint, NULL);
		delete mapcon;
		delete schema_constraint;
						
		peer->lookup_forward = mklist("lookup");
		peer->lookup_backward = mklist("lookup");
		peer->layer = new svector();
		peer->container = new pvector();
		
		int schema_match = peer_schema->construct_lookup(mp->msg_schema, constraints, peer->lookup_forward, peer->lookup_backward, 
															peer->container, peer->layer);

		if (peer->layer->count() == 0)
		{
			delete peer->layer;
			peer->layer = NULL;
		}
		if (peer->container->count() == 0)
		{
			delete peer->container;
			peer->container = NULL;
		}
		
		delete constraints;

		if (schema_match == 0)
		{
			warning("Peer's schema does not fit map constraint '%s' - unmapping\n", peer->map_constraint);
			unmap(mp, peer->address, peer->endpoint, -1);
		}
		else
		{
			// If we have a subscription criteria.
			if (mp->subs != NULL)
				// Resubscribe, this will convert the subscription string to peer's schema.
				peer->resubscribe(mp->subs, mp->topic);
		}
	}
}

void swrapper::map_report(int report_fd, int code, const char *address)
{
	sreturncode *ret;

	// Report success or failure from mapping operation:
	if(report_fd >= 0)
	{
		ret = new sreturncode();
		ret->retcode = code;
		ret->address = sdup(address);
		if(ret->write(report_fd) < 0)
			error("Error sending map failure report to library");
		delete ret;
	}
}

void swrapper::resolve_address(const char *addrstring, mapparams *params)
{
	snode *sn;
	MapConstraints *mapcon;
	const char *rdc_address;
	int ok;
	
	mapcon = new MapConstraints(addrstring);	
	sn = mklist("criteria");
	if (params->mp->flexible_matching)
		sn->append(mapcon->pack(params->mp->msg_schema->hashes));
	else
		sn->append(mapcon->pack());
	sn->append(params->mp->pack_interface(1));
	delete mapcon;

	params->map_constraint = sdup(addrstring);
	params->query_sn = sn;
		
	params->remaining_rdcs = rdc->count();

	for(int i = 0; i < rdc->count(); i++)
	{
		rdc_address = rdc->item(i);
		// Incredibly dirty hack because somehow the reply from the phone RDC doesn't work in continue_dispose.
		#ifdef __ANDROID__
			if (!strcmp(rdc_address, "localhost:50123"))
			{
				params->remaining_rdcs--;
				continue;
			}
		#endif
		ok = begin_visit(VisitResolveConstraints, rdc_address, "rdc",
				"lookup_cpt", lookup_cpt_mp, params->query_sn,
				params, NULL, NULL);
		if(ok < 0)
		{
			log("Note: could not consult RDC at %s", rdc_address);
			params->remaining_rdcs--;
			sassert(params->remaining_rdcs >= 0,
					"Negative number of remaining RDCs in resolve constraints");
		}
		// Otherwise, now need to wait for a reply from that one
	}
	
	continue_resolve(params);
}

void swrapper::continue_resolve(mapparams *params)
{
	sassert(params->remaining_rdcs >= 0,
			"Negative number of remaining RDCs in resolve constraints");

	if(params->remaining_rdcs > 0) // Not ready yet
		return;
	if (params->operation == RSMap)
		do_map(params);
	else if (params->operation == RSUnmap)
		do_unmap(params);
	else if (params->operation == RSDivert)
		do_divert(params);
		
}

//avoid calling-out to the RDC if we're only conce
void swrapper::resolve_address_local(smidpoint *mp, const char *addrstring, mapparams *params)
{
	snode *constraints;
	MapConstraints *mapcon;
	const char *rdc_address;
	int ok;

	//get the map constraints
	mapcon = new MapConstraints(addrstring);
	constraints = mapcon->pack();

	//printf("The constraint structure is like this %s\n", constraints->toxml(1));

	//compare constraints against all peers...
	for (int i = 0; i < mp->peers->count(); i++)
	{

		//TODO: Only matching on name/instance, for now... creator, keywords TODO!
		if(constraints->exists("cpt-name"))
			if (strcmp(mp->peers->item(i)->cpt_name, constraints->extract_txt("cpt-name")))
					continue;

		if(constraints->exists("instance-name"))
			if (strcmp(mp->peers->item(i)->cpt_name, constraints->extract_txt("instance-name")))
					continue;

		//here we have a match - so add it.
		params->local_possibilities->add_noduplicates(mp->peers->item(i)->address);
		//printf("Found a possibility %s\n",mp->peers->item(i)->address);
	}

}


void swrapper::load_privileges_from_file(const char *filenm){

	int num_entries;
	const char *err;
	snode **input;
	Schema *schema;

	log("Loading permissions from file %s\n",filenm);
	input = snode::import_file_multi(filenm, &num_entries, &err);
	if(input == NULL)
		error("Error reading XML from %s:\n%s\n", filenm, err);

	//convert to a_c schema
	schema = Schema::create(access_control_schema_str, &err);
	if (schema==NULL)
			error("Error reading schema %s\n", err);

	for (int i = 0; i < num_entries; i++)
	{
		if(!validate(input[i], schema, &err))
			error("Permission in file fails to confirm with the access_control schema:\n%s\n", err);
		privilegeparams *prv;
		const char *ep;
		if (input[i]->exists("target_ept"))
			ep = input[i]->extract_txt("target_ept");
		else
			ep = strdup("");
		prv = new privilegeparams(ep, input[i]->extract_txt("principal_cpt"), input[i]->extract_txt("principal_inst"), input[i]->extract_flg("authorised"));
		//printf("about to process permission on ept %s, for %s:%s, adding? %d\n",prv->target_endpt,prv->p_cpt,prv->p_inst,prv->addpriv);
		change_privileges(prv);
	}

}

void swrapper::change_privileges(privilegeparams *params)
{
	spermissionvector *pv;
	smidpoint *mp;
	spermission *perms = new spermission(params->p_cpt,params->p_inst);
	int modified = false;
	//add the privileges to all the endpoints
	for(int i = 0; i < mps->count(); i++)
	{
		mp = mps->item(i);
		if (!strcmp("",params->target_endpt) || !strcmp(mp->name,params->target_endpt)) 		//apply to this endpoint (Null => all)
			if (params->addpriv)
			{
				mp->acl_ep->add(perms);
				log("Wrapper: Added permission %s:%s on ept '%s'\n",perms->principal_cpt,perms->principal_inst,mp->name);
				modified = true;
			}
			else
			{
				mp->acl_ep->delete_permission(perms);
				log("Wrapper: deleted permission %s:%s on ept '%s'\n",perms->principal_cpt,perms->principal_inst,mp->name);
				modified = true;

				//if we've removed a privilege, re-evaluate the connections...
				int j = 0;
				while (j < mp->peers->count())
				{
					if (!mp->acl_ep->check_authorised(mp->peers->item(j)->cpt_name,mp->peers->item(j)->instance))
					{
						printf("Wrapper: Connected peer %s:%s is *no longer* authorised. Killing his connection\n",mp->peers->item(j)->cpt_name,mp->peers->item(j)->instance);
						departure(mp->peers->item(j), false);
					}
					else
						j++;
				}

			}
	}

	if (modified && register_with_rdc)
		update_privileges_on_rdc(params);

	if (!params->addpriv)
		delete perms;

	delete params;
}

void swrapper::update_privileges_on_rdc(privilegeparams *params, const char *address)
{
	const char *rdc_address;
	snode *sn;
	int ok;

	//an rdc shouldnt pass this info on...
	if (!strcmp("rdc",cpt_name))
		return;

	//build up the permissions node
	sn = pack(pack(cpt_name, "target_cpt"),
			pack(instance_name, "target_inst"),
			pack(canonical_address, "target_address"),
			pack(params->target_endpt, "target_endpt"),
			pack(params->p_cpt, "principal_cpt"),
			pack(params->p_inst, "principal_inst"),
			pack_bool(params->addpriv, "add_perm"),
			"event");

	if (address == NULL)
	{
		//printf("Going to send %s to the rdc\n",sn->toxml(0));
		for(int i = 0; i < rdc->count(); i++)
		{
			rdc_address = rdc->item(i);
			if(rdc_address[0] == '!')
				continue;

			ok = begin_visit(VisitUpdatePrivilege, rdc_address, "rdc", "set_acl",
					rdcacl_mp, sn, NULL, NULL, NULL);
			if(ok < 0)
				log("Note: could not send update privilege message to RDC at %s", rdc_address);
		}
	}
	else
	{
		rdc_address = address;
			if(rdc_address[0] == '!')
				return;
				
		ok = begin_visit(VisitUpdatePrivilege, rdc_address, "rdc", "set_acl",
					rdcacl_mp, sn, NULL, NULL, NULL);
		if(ok < 0)
				log("Note: could not send update privilege message to RDC at %s", rdc_address);
	}
}


mapparams::mapparams()
{
	possibilities = new svector();
	local_possibilities = new svector();
	query_sn = NULL;
	endpoint = NULL;
	map_constraint = NULL;
}

mapparams::~mapparams()
{
	delete possibilities;
	delete local_possibilities;
	if(query_sn != NULL) delete query_sn;
	if(endpoint != NULL) delete[] endpoint;
	if(map_constraint != NULL) delete[] map_constraint;
}

void swrapper::map(smidpoint *mp, const char *addrstring, const char *endpoint,
		int report_fd)
{
	// report_fd is -1 if no report to be sent
		
	int address_type;
	mapparams *params;
	
	params = new mapparams();
	params->mp = mp;
	params->endpoint = sdup(endpoint);
	params->report_fd = report_fd;
	params->operation = RSMap;

	address_type = MapConstraints::is_constraint(addrstring);
	if(address_type == -1)
	{
		delete params;
		map_report(report_fd, 0);
	}
	else if(address_type == 1)
		resolve_address(addrstring, params);
	else if(address_type == 0)
	{
		params->possibilities->add(addrstring);
		do_map(params);
	}
}

void swrapper::continue_mapping(int fd)
{
	;
}

void swrapper::do_map(mapparams *params)
{
	int remote_fd;
	shello *hello;
	AbstractMessage *abst;
	speer *peer;
	const char *addr;
	int already_mapped;
	
	smidpoint *mp = params->mp;
	svector *possibilities = params->possibilities;
	char *endpoint = sdup(params->endpoint);
	char *map_constraint = sdup(params->map_constraint);
	int report_fd = params->report_fd;

	remote_fd = -1;
	for(int i = 0; i < possibilities->count(); i++)
	{
		addr = possibilities->item(i);
		// Check if already mapped:
		already_mapped = 0;
		for(int j = 0; j < mp->peers->count(); j++)
		{
			peer = mp->peers->item(j);
	
			if(!strcmp(peer->address, possibilities->item(i)))
			{
				// Has "localhost" been converted to 127.0.0.1 if needed? - XXX
				already_mapped = 1;
				break;
			}
		}
		if(already_mapped)
		{
			warning("Tried to map to %s, but already mapped", addr);
			continue;
		}
				
		remote_fd = activesock(addr);
		if(remote_fd != -1)
		{
			// printf("Contacted target at address %s\n", addr);
			break;
		}
	}
	delete params;
	if(remote_fd == -1)
	{
		delete[] endpoint;
		map_report(report_fd, 0);
		return;
	}
	if(nonblocking)
		sock_noblock(remote_fd);
	
	// Create hello message:
	hello = new shello();
	hello->source = sdup(cpt_name);
	hello->from_address = sdup(canonical_address);
	hello->from_instance = sdup(instance_name);
	hello->from_endpoint = sdup(mp->name);
	hello->from_ep_id = mp->ep_id;
	hello->target = NULL; // We don't specify
	hello->flexible_matching = mp->flexible_matching;
	hello->required_endpoint = endpoint; // Will delete for us
	hello->required_ep_id = 0; // No means of specifying this in mapparams yet
	switch(mp->type)
	{
		case EndpointSink: hello->target_type = EndpointSource; break;
		case EndpointSource: hello->target_type = EndpointSink; break;
		case EndpointClient: hello->target_type = EndpointServer; break;
		case EndpointServer: hello->target_type = EndpointClient; break;
		default:
				error("Unknown endpoint type in map()");
	}
	if(mp->subs != NULL)
		hello->subs = sdup(mp->subs);
	if(mp->topic != NULL)
		hello->topic = sdup(mp->topic);
	hello->msg_hc = (mp->msg_hc == NULL) ? new HashCode() : new HashCode(mp->msg_hc);
	hello->reply_hc = (mp->reply_hc == NULL) ? new HashCode() : new HashCode(mp->reply_hc);

	// Create a skeletal speer structure:
	peer = new speer();
	peer->owner = mp;
	peer->map_constraint = map_constraint;
	
	// Preselect to send the hello message:
	abst = hello->wrap(remote_fd);
	delete hello;
	abst->peer = peer;
	abst->report_fd = report_fd;
	preselect_fd->add(remote_fd);
	preselect_direction->add(FDOutbound);
	multi->add(remote_fd, MULTI_WRITE, "swrapper::do_map");
	AbstractMessageQueue *q = progress_out[remote_fd];
	sassert(q->isempty(), "progress_out queue not empty");
	q->add(abst);
	fdstate[remote_fd] = FDGreeting;
}

void swrapper::finalise_map(int fd, AbstractMessage *abst)
{
	AcceptanceCode code;
	swelcome *welcome;
	int ret;
	speer *peer;
	int report_fd;
	int discon = 0;
	
	peer = abst->peer;
	report_fd = abst->report_fd;
	
	welcome = new swelcome();
	ret = welcome->reveal(abst);	
	if(ret < 0)
	{
		warning("Wrong message type whilst expecting welcome message");
		map_report(report_fd, 0);
		discon = 1;
	}
	else
	{
		code = welcome->code;
		if(code == AcceptAlreadyMapped)
		{
			map_report(report_fd, 1, welcome->address);
			discon = 1;
		}
		else if(code != AcceptOK)
		{
			// TODO: make this return the acceptance code instead of a boolean
			map_report(report_fd, 0);
			discon = 1;
		}
	}
	if(discon)
	{
		fdstate[fd] = FDUnused;
		multi->remove(fd, MULTI_READ);
		progress_in[fd] = NULL;
		// delete abst;
		delete welcome;
		delete peer;
		close(fd);
		return;
	}
	
	peer->uid = next_uid++;
	peer->sock = fd;
	peer->cpt_name = sdup(welcome->cpt_name);
	peer->instance = sdup(welcome->instance);
	peer->endpoint = sdup(welcome->endpoint);
	peer->address  = sdup(welcome->address);
	if(welcome->topic != NULL)
		peer->topic = sdup(welcome->topic);
	if(welcome->subs != NULL)
	{
		peer->subs = new subscription(welcome->subs);
		/*
		printf("Parsed subscription:\n");
		peer->subs->dump_tokens();
		peer->subs->dump_tree();
		*/
	}
	peer->msg_poly = welcome->msg_poly;
	peer->reply_poly = welcome->reply_poly;
	
	if (peer->owner->flexible_matching && welcome->msg_hc->equals(peer->owner->msg_hc) == 0)
	{
		Schema *sch = cache->lookup(welcome->msg_hc);
		if (sch == NULL)
		{
			// Don't know schema, let's go and get it.
			
			// Initiate schema lookup:
			snode *sn;
			int ok;
			const char *s;

			s = welcome->msg_hc->tostring();
			sn = pack(s, "hashcode");
			delete[] s;
			
			// Put a message with the peer uid into the AbstractMessage.
			// Then when we get results of lookup_schema, we can find the peer again,
			// so we can go and make a lookup table.
			scomm *msg = new scomm();
			msg->peer_uid = peer->uid;
			abst->unrecognised = msg;
			
			ok = begin_visit(VisitLookupSchema, peer->address, peer->cpt_name,
					"lookup_schema", lookup_schema_mp, sn, NULL, NULL, msg);
			if(ok < 0)
			{
				/* Couldn't make contact back to component, hence lookup
					isn't going ahead. Without the schema, we can't process
					the original message. */
				warning("Warning: schema lookup on component '%s' failed", peer->address);
				warning("Cannot construct lookup table without relevant schema");
			}
		}
		else
		{
			// We already know schema, construct lookup table.
			construct_peer_lookup(peer->owner, peer, sch);
		}
	}
	
	delete welcome;
	
	peer->owner->peers->add(peer);
	multi->add(peer->sock, MULTI_READ, "swrapper::finalise_map");
	fdstate[peer->sock] = FDPeer;
	// delete abst;

	// Report mapping successful:
	map_report(report_fd, 1, peer->address);
}

// report_fd is -1 if no report to be sent
void swrapper::unmap(smidpoint *mp, const char *addrstring, const char *endpoint, int report_fd)
{
	
	int address_type;
	mapparams *params;
	
	params = new mapparams();
	params->mp = mp;
	params->operation = RSUnmap;
	if (endpoint !=NULL)
		params->endpoint = sdup(endpoint);
	params->report_fd = report_fd;
	if  (addrstring==NULL)
		addrstring = address_wildcard; //hack placeholder replacing NULL to mean disconnect all...
	address_type = MapConstraints::is_constraint(addrstring);
	if(address_type == -1)
	{
		delete params;
		map_report(report_fd, 0);
	}
	else if(address_type == 1)
	{
		//printf("8912498 in unmap - trying to resolve addr %s\n",addrstring);
		resolve_address_local(mp, addrstring, params);
	}
	else if(address_type == 0)
	{
		params->local_possibilities->add(addrstring);
	}
	do_unmap(params);
}



// report_fd is -1 if no report to be sent
void swrapper::do_divert(mapparams *params)
{

	speer *peer;
	char *address;
	smidpoint *mp = params->mp;
	svector *possibilities = params->local_possibilities;
	for(int i = 0; i < possibilities->count(); i++)
	{
		address = possibilities->item(i);	
		for(int i = 0; i < mp->peers->count(); i++){
			peer = mp->peers->item(i);
			if ( (!strcmp(address,address_wildcard) || !strcmp(address, peer->address)) && ( params->endpoint==NULL || !strcmp(params->endpoint, peer->endpoint) ) )		
			{
						peer->divert(params->newaddr, params->newendpt);
						log("Diverted peer %s:%s to %s:%s",peer->address, peer->endpoint, params->newaddr, params->newendpt);
			}
		}
		delete params;
	}
}



// report_fd is -1 if no report to be sent
void swrapper::do_unmap(mapparams *params)
{
	int remote_fd;
	sgoodbye *shutdown;
	AbstractMessage *abst;
	speer *peer;
	char *address;
	int size, index;
	smidpoint *mp = params->mp;
	svector *possibilities = params->local_possibilities;
	
	//	char *endpoint = params->endpoint;
	int report_fd = params->report_fd;
	for(int i = 0; i < possibilities->count(); i++)
	{
		address = possibilities->item(i);
		size = mp->peers->count();
		index = 0;
		while(index < size)
		{
			peer = mp->peers->item(index);
			//only disconnect the specified endpoint - NULLs => apply to all...
			if ( (!strcmp(address,address_wildcard) || !strcmp(address, peer->address)) && ( params->endpoint==NULL || !strcmp(params->endpoint, peer->endpoint) ) )
			{
				// Send a clean disconnect message:
				log("Closing connection on endpoint:%s with %s(%s):%s\n",mp->name,peer->cpt_name,peer->address,peer->endpoint);
				shutdown = new sgoodbye();
				shutdown->src_cpt = sdup(cpt_name);
				shutdown->src_ep = sdup(mp->name);
				shutdown->tgt_cpt = sdup(peer->cpt_name);
				shutdown->tgt_ep = sdup(peer->endpoint);
				abst = shutdown->wrap(peer->sock);
				abst->blockadvance(timeout_unmap); // No use checking for errs here
				delete abst;
				delete shutdown;		
				departure(peer, 0);
				size--;
			}
			else
				index++;
		}
	}
	delete params;
	map_report(report_fd, 0);
}


void swrapper::ismap(smidpoint *mp, int report_fd)
{
	map_report(report_fd, mp->peers->count());
}

void swrapper::serve_boot()
{
	sstopwrapper *stop;
	saddendpoint *add;
	shook *hook;
	srdc *rdc_message;
	int ret;

	stop = new sstopwrapper();
	add = new saddendpoint();
	hook = new shook();
	rdc_message = new srdc();
	ret = read_bootupdate(bootstrap_fd, add, stop, hook, rdc_message);
	
	if(ret == -2)
	{
		error("Only valid messages on bootstrap pipe after start are "
				"add, stop, getstatus, getschema, declare");
	}
	else if(ret < 0)
	{
		error("Bootstrap connection to library broken");
	}
	else if(ret == MessageAddEndpoint)
	{
		smidpoint *mp;
		Schema *sch;
		
		mp = add_endpoint(add);
		
		if(!strcmp(mp->name, "rdc_update"))
		{
			rdc_update_notify = true;
			rdc_update_mp = mp;
		}
		
		sch = cache->lookup(add->msg_hc);
		if(sch == NULL)
		{
			error("Tried to add endpoint %s for which message schema has not "
					"been declared.", add->endpoint);
		}
		mp->msg_schema = new Schema(sch);
		sch = cache->lookup(add->reply_hc);
		if(sch == NULL)
		{
			error("Tried to add endpoint %s for which reply schema has not "
					"been declared.", add->endpoint);
		}
		mp->reply_schema = new Schema(sch);
		multi->add(mp->fd, MULTI_READ, "swrapper::serve_boot");
	}
	else if(ret == MessageStop)
	{
		log("Wrapper instructed to stop by the application");
		if (register_with_rdc)
			register_cpt(0);

		exit(0);
	}
	else if(ret == MessageGetStatus)
	{
		sgeneric *status;
		
		status = new sgeneric(MessageStatus);
		status->hc = new HashCode(get_status_mp->reply_hc);
		status->tree = pack_status();
		status->write(bootstrap_fd);
		delete status;
	}
	else if(ret == MessageDeclare)
	{
		const char *schema;
		int file_lookup;
		Schema *sch;
		sgeneric *sg;
		const char *s;

		schema = hook->tree->extract_txt("schema");
		file_lookup = hook->tree->extract_flg("file_lookup");
		sch = declare_schema(schema, file_lookup);
		s = sch->hc->tostring();
		
		sg = new sgeneric(MessageHash);
		sg->hc = new HashCode();
		sg->hc->fromstring("D3C74D1897A3"); /* @txt hash */
		sg->tree = pack(s, "hash");
		delete[] s;
		
		sg->write(bootstrap_fd);
		delete sg;
	}
	else if(ret == MessageGetSchema)
	{
		sgeneric *sg;
		const char *hash;
		Schema *schema;
		HashCode *hc;
		const char *s;
		
		hash = hook->tree->extract_txt();
		hc = new HashCode();
		hc->fromstring(hash);
		schema = cache->lookup(hc);
		delete hc;
		
		sg = new sgeneric(MessageSchema);
		sg->hc = new HashCode();
		sg->hc->fromstring("D39E44946A6C"); /* @txt schema */
		
		if(schema == NULL)
		{
			sg->tree = pack("?", "schema");
		}
		else
		{
			s = schema->canonical_string();
			sg->tree = pack(s, "schema");
			delete[] s;
		}
		
		sg->write(bootstrap_fd);
		delete sg;
	}
	else if(ret == MessageRdc)
	{
		if (rdc_message->address != NULL && (rdc_message->arrived == 0 || rdc_message->arrived == 1))
		{
			// register/deregister with the new rdc.
			handle_new_rdc(rdc_message->arrived, rdc_message->address);
		}
		if (rdc_message->notify == 0 || rdc_message->notify == 1)
		{
			rdc_update_notify = rdc_message->notify;
		}
		if (rdc_message->autoconnect == 0 || rdc_message->autoconnect == 1)
		{
			rdc_update_autoconnect = rdc_message->autoconnect;
		}
	}

	delete stop;
	delete add;
	delete hook;
	delete rdc_message;
}

void swrapper::serve_endpoint(AbstractMessage *abst, smidpoint *mp)
{
	sinternal *sint;
	
	sint = new sinternal();
	if(sint->reveal(abst) < 0)
	{
		error("Protocol error: wrong message type %d received from library",
				abst->get_type());
	}
	if(sint->oob_ctrl != NULL)
	{
		scontrol *ctrl = sint->oob_ctrl;
		int reply_fd;
		
		reply_fd = mp->fd;
		// reply_fd = bootstrap_fd;
		
		switch(sint->type)
		{
			case MessageMap:
				map(mp, ctrl->address, ctrl->target_endpoint, reply_fd);
				break;
			case MessageUnmap: 
				unmap(mp, ctrl->address, ctrl->target_endpoint, reply_fd); break;
			case MessageIsmap: ismap(mp, reply_fd); break;
			case MessageSubscribe:
				subscribe(mp, ctrl->subs, ctrl->topic, ctrl->peer);
				break;
			case MessagePrivilege:
				//printf("Received a privilege event...ep='%s' cpt='%s' inst='%s' add='%s' \n", ctrl->target_endpoint, ctrl->principal_cpt, ctrl->principal_inst, ctrl->adding_permission?"ADDING":"REMOVING");
				//printf("******In serve endpoint fo msg priv\n");
				privilegeparams *prv;
				prv = new privilegeparams(ctrl->target_endpoint, ctrl->principal_cpt, ctrl->principal_inst, ctrl->adding_permission);
				change_privileges(prv);
				break;
			case MessageLoadPrivileges:
				//printf("Wrapper received an instruction to load privileges from the file %s\n",ctrl->filename);
				load_privileges_from_file(ctrl->filename);
				break;
			case MessageMapPolicy:
				snode *event, *endpoint, *peer_address, *peer_endpoint, *certificate, *create;
				endpoint = pack(mp->name, "endpoint");
				peer_address = pack(ctrl->address, "peer_address");
				peer_endpoint = pack(ctrl->target_endpoint, "peer_endpoint");
				certificate = pack("", "certificate");
				create = pack_bool(1, "create");
				event = pack(endpoint, peer_address, peer_endpoint, certificate, create, "event");
				
				// TODO: make a new Visit type - just hopping on this one the moment (but it works).
				const char *rdc_address;
				int ok;
				for(int i = 0; i < rdc->count(); i++)
				{
					rdc_address = rdc->item(i);
					if(rdc_address[0] == '!')
						continue;

					begin_visit(VisitUpdatePrivilege, "localhost:50123", "rdc", "map_policy",
							map_policy_mp, event, NULL, NULL, NULL);
					if(ok < 0)
						log("Note: could not send map policy message to RDC at %s", rdc_address);
				}
						
				break;
			default:
				error("Unknown control message");
		}
	}
	else
	{
		switch(sint->type)
		{
			case MessageEmit:
				mp->emit(sint->topic, sint->xml, sint->hc);
				mp->processed++;
				break;
			case MessageRPC:
				mp->rpc(sint->xml, sint->hc);
				break;
			case MessageReply:
				mp->reply(sint->seq, sint->xml, sint->hc);
				break;
			default:
				error("Unknown internal message type");
		}
	}
	delete sint;
}

/* smidpoint */

void smidpoint::emit(const char *topic, const char *xml, HashCode *hc)
{
	// Send scomm (MessageSink)
	const char *err;
	unsigned char *buf;
	int length;
	scomm *msg;
	speer *peer;
	snode *sn;
	Schema *schema;

	if(type != EndpointSource)
		error("Tried to emit from a non-source endpoint");

	sn = snode::import(xml, &err);
	if(sn == NULL)
		error("Failed to parse internal XML:\n%s", err);

	// Check hc:
	if(msg_hc->ispolymorphic())
	{
		if(hc->ispolymorphic())
		{
			error("Emitted messages must have a definite type, not a"
					" polymorphic one");
		}
		schema = wrap->cache->lookup(hc);
		if(schema == NULL)
		{
			error("Application specified unknown message hash code %s to "
					"polymorphic endpoint <%s>", hc->tostring(), name);
		}
	}
	else
	{
		if(!hc->equals(msg_hc))
		{
			error("Application specified incorrect message hash code %s to "
					"fixed-type endpoint <%s>", hc->tostring(), name);
		}
		schema = msg_schema;
	}
		
	buf = marshall(sn, schema, &length, &err);
	if(buf == NULL)
		error("Message emitted does not conform with schema:\n%s", err);
	msg = new scomm();
	
	// Init msg:
	msg->source = sdup(*cpt_name);
	msg->src_endpoint = sdup(name);
	msg->topic = sdup(topic);
	msg->type = MessageSink;
	msg->seq = 0;
	msg->hc = new HashCode(hc);
	msg->length = length;
	msg->data = buf;
	
	for(int i = 0; i < peers->count(); i++)
	{
		peer = peers->item(i);
		// Check subscription (peer->subs, peer->topic):
		if(peer->subs != NULL && !peer->subs->match(sn))
		{
			// printf("Subscription doesn't match\n");
			continue; // Subscription doesn't match
		}
		if(peer->topic != NULL && topic != NULL && strcmp(topic, peer->topic))
		{
			// printf("Topic doesn't match\n");
			continue; // Topic doesn't match
		}
		
		if(msg->target != NULL) delete[] msg->target;
		if(msg->tgt_endpoint != NULL) delete[] msg->tgt_endpoint;
		msg->target = sdup(peer->cpt_name);
		msg->tgt_endpoint = sdup(peer->endpoint);

		peer->deliver_remote(msg, (topic != NULL && !strcmp(topic, "disrupt")) ?
				1 : 0);
	}
	delete msg; // msg will delete buf for us
	delete sn;
}

void smidpoint::rpc(const char *xml, HashCode *hc)
{
	// Send scomm (MessageServer)
	// Make a note in issued_rpcs to expect a reply (MessageClient)
	snode *sn;
	const char *err;
	unsigned char *buf;
	int length;
	scomm *msg;
	speer *peer;
	Schema *schema;

	if(type != EndpointClient)
		error("Tried to RPC from a non-client endpoint");
		
	if(peers->count() != 1)
	{
		smessage *unavailable;
		
		unavailable = new smessage();
		unavailable->type = MessageUnavailable;
		deliver_local(unavailable);
		delete unavailable;
		return;
	}
	peer = peers->item(0);

	// Check hc:
	if(msg_hc->ispolymorphic())
	{
		schema = wrap->cache->lookup(hc);
		if(schema == NULL)
		{
			error("Application specified unknown message hash code %s to "
					"polymorphic endpoint <%s>", hc->tostring(), name);
		}
	}
	else
	{
		if(!hc->equals(msg_hc))
		{
			error("Application specified incorrect message hash code %s to "
					"fixed-type endpoint <%s>", hc->tostring(), name);
		}
		schema = msg_schema;
	}
		
	sn = snode::import(xml, &err);
	if(sn == NULL)
		error("Failed to parse internal XML:\n%s", err);
	buf = marshall(sn, schema, &length, &err);
	if(buf == NULL)
		error("RPC message does not conform with schema:\n%s", err);
	
	msg = new scomm();
	msg->source = sdup(*cpt_name);
	msg->src_endpoint = sdup(name);
	msg->target = sdup(peer->cpt_name);
	msg->tgt_endpoint = sdup(peer->endpoint);
	msg->type = MessageServer;
	msg->seq = next_seq++;
	msg->hc = new HashCode(hc);
	msg->length = length;
	msg->data = buf;
	
	// Add entry to issued_rpcs:
	smessage *later;
	later = new smessage();
	later->type = MessageResponse;
	later->source_cpt = sdup(peer->cpt_name);
	later->source_inst = sdup(peer->instance);
	later->source_ep = sdup(peer->endpoint);
	later->source_ep_id = peer->ep_id;
	later->seq = msg->seq;
	later->hc = new HashCode(reply_hc); // May be overriden later if polymorph
	/* When the server replies, we will just need to fill in
		snode *later->tree and then the response can be sent back
		to the library */
	issued_rpcs->add(later);

	peer->deliver_remote(msg);	
	delete msg; // msg will delete buf for us
	delete sn;
}

void smidpoint::reply(int seq, const char *xml, HashCode *hc)
{
	// Send scomm (MessageClient), consuming one of pending_replies
	snode *sn;
	const char *err;
	unsigned char *buf;
	int length;
	scomm *msg;
	Schema *schema;

	if(type != EndpointServer)
		error("Tried to reply from a non-server endpoint");
	
	msg = pending_replies->preview();
	if(msg == NULL)
	{
		warning("Reply without a request; discarding it");
		return;
	}
	if(msg->seq != seq)
	{
		warning("Reply does not match request sequence number; discarding it");
		return;
	}
	pending_replies->remove();
	
	// Check hc:
	if(reply_hc->ispolymorphic())
	{
		schema = wrap->cache->lookup(hc);
		if(schema == NULL)
		{
			error("Application specified unknown message hash code %s to "
					"polymorphic endpoint <%s>", hc->tostring(), name);
		}
	}
	else
	{
		if(!hc->equals(reply_hc))
		{
			error("Application specified incorrect message hash code %s to "
					"fixed-type endpoint <%s>", hc->tostring(), name);
		}
		schema = reply_schema;
	}
		
	// Convert xml, to fill in msg->data and msg->length:
	sn = snode::import(xml, &err);
	if(sn == NULL)
		error("Failed to parse internal XML:\n%s", err);
	buf = marshall(sn, schema, &length, &err);
	if(buf == NULL)
		error("Reply message does not conform with schema:\n%s", err);
	msg->data = buf;
	msg->length = length;
	if(reply_hc->ispolymorphic())
		msg->hc = new HashCode(hc);

	int index;
	speer *peer;
	for(index = 0; index < peers->count(); index++)
	{
		peer = peers->item(index);
		if(peer->uid == msg->peer_uid)
			break;
	}
	if(index == peers->count())
	{
		error("Peer disconnected after query and before we could reply");
		// This will definitely be made into a silent-continue case later
	}
	peer->deliver_remote(msg);
	delete msg; // msg will delete buf for us
}

smidpoint::smidpoint()
{
	name = NULL;
	msg_schema = reply_schema = NULL;
	msg_hc = reply_hc = NULL;
	subs = topic = NULL;
	peers = new speervector();
	issued_rpcs = new smessagequeue();
	pending_replies = new scommqueue();
	next_seq = 0;
	processed = waiting = dropped = 0;
	ep_id = 0; // Must be filled in
	flexible_matching = 0;
	acl_ep = new spermissionvector();
}

smidpoint::~smidpoint()
{
	for(int i = 0; i < peers->count(); i++)
		delete peers->item(i);
	delete peers;

	for(int i = 0; i < acl_ep->count(); i++)
		delete acl_ep->item(i);
	delete acl_ep;

	delete issued_rpcs;
	delete pending_replies;
	
	if(msg_hc != NULL) delete msg_hc;
	if(reply_hc != NULL) delete reply_hc;
	if(msg_schema != NULL) delete msg_schema;
	if(reply_schema != NULL) delete reply_schema;
	if(name != NULL) delete[] name;
	if(subs != NULL) delete[] subs;
	if(topic != NULL) delete[] topic;
}

char *smidpoint::verify_metadata(snode *sn)
{
	const char *idl;
	const char *err;
			
	if(strcmp(name, sn->extract_txt("name")))
	{
		return sformat("Endpoint names '%s' and '%s' differ",
				name, sn->extract_txt("name"));
	}
	if(type >= NUM_ENDPOINT_TYPES ||
			sn->extract_enum("type") >= NUM_ENDPOINT_TYPES)
	{
		return sformat("Endpoint '%s' has out-of-range type number %d or %d",
				name, type, sn->extract_enum("type"));
	}
	if(type != (EndpointType)sn->extract_enum("type"))
	{
		return sformat("Endpoint '%s' declares type %s, metadata requires %s",
				name, endpoint_type[type], endpoint_type[sn->extract_enum("type")]);
	}
	
	// Read message schema:
	idl = sn->extract_txt("message");
	msg_schema = Schema::create(idl, &err);
	if(msg_schema == NULL)
		error("Endpoint '%s' message schema is invalid:\n%s", name, err);
	
	// Check schema hashes to same value as supplied by constructor:
	if(!msg_schema->hc->equals(msg_hc))
	{
		return sformat("Component message schema for endpoint '%s' differs from "
				"definition", name);
	}
	
	// Read reply schema:
	idl = sn->extract_txt("response");
	reply_schema = Schema::create(idl, &err);
	if(reply_schema == NULL)
		error("Endpoint '%s' reply schema is invalid:\n%s", name, err);

	// Check schema hashes to same value as supplied by constructor:
	if(!reply_schema->hc->equals(reply_hc))
	{
		return sformat("Component response schema for endpoint '%s' differs "
				"from definition", name);
	}

	wrap->cache->add(msg_schema);
	wrap->cache->add(reply_schema);
		
	return NULL;
}

/* speer */

void smidpoint::deliver_local(smessage *msg)
{
	AbstractMessage *abst;
	AbstractMessageQueue *q;
	
	abst = msg->wrap(fd);
	q = wrap->progress_out[fd];
	if(!q->isempty())
		q->add(abst); // Already a message being sent
	else
	{
		wrap->preselect_fd->add(fd);
		wrap->preselect_direction->add(FDOutbound);
		wrap->multi->add(fd, MULTI_WRITE, "smidpoint::deliver_local");
		q->add(abst);
	}
}
	
void speer::sink(snode *sn, HashCode *hc, const char *topic)
{
	// return to library smessage: MessageRcv
	smessage *msg;
	
	msg = new smessage();
	msg->type = MessageRcv;
	msg->source_cpt = sdup(cpt_name);
	msg->source_inst = sdup(instance);
	msg->source_ep = sdup(endpoint);
	msg->topic = sdup(topic);
	msg->source_ep_id = ep_id;
	msg->seq = 0;
	msg->hc = new HashCode(hc);
	// We use actual msg hash, not owner->msg_hc, as this might be polymorphic
	
	// If we support partial matching, the schemas are actually different, and we have a lookup table.
	if (owner->flexible_matching && owner->msg_hc->equals(msg->hc) == 0 && lookup_backward != NULL)
	{
		snode *repacked;
		snode *node;
		
		node = sn;
				
		// In this case, the other schema is bigger - follow the path to our top level node.
		if (layer != NULL)
			node = sn->follow_path(layer);

		// Repack the message using our names.
		// Skip any fields which are not in the lookup table.
		// Add any of our fields which the message does not have.
		repacked = repack(node);
							
		// In this case, our schema is bigger than the other schema.	
		if (container != NULL)
		{
			svector *level;
			const char *level_name;
			// Loop through the outer layers, from closest to us to the top level.
			for (int i = 0; i < container->count(); i++)
			{
				// Each item contains the name of the level, plus any additional fields we expect.
				level = (svector *)container->item(i);
				level_name = level->item(0);
				snode *parent = mklist(level_name);
				
				// Iterate through the additional expected fields.
				for (int j = 1; j < level->count(); j++)
				{
					// If the child has the same name as our node, pack it, otherwise pack an empty node.
					if (!strcmp(level->item(j), repacked->get_name()))
						parent->append(repacked);
					else
						parent->append(pack((const char *)NULL, level->item(j)));
				}
				
				repacked = parent;
			}
		}

		delete sn;

		msg->tree = repacked;
	}
	
	if (msg->tree == NULL)
		msg->tree = sn; // Consumes sn
		
	owner->deliver_local(msg);
	delete msg;
}

snode *speer::repack(snode *sn)
{
	const char *local_name;
	snode *scomposite, *child, *top;
	int children = 0;
	
	// If there's no entry for what we call this field, skip it.
	if (lookup_backward->exists(sn->get_name()))
		local_name = lookup_backward->extract_txt(sn->get_name());
	else
		return NULL;
	
	if (sn->get_type() == SStruct || sn->get_type() == SList)
		scomposite = mklist(local_name);
	
	for (int i = 0; i < sn->count(); i++) {	

		// If there's no entry for what we call this field, skip it.
		if (lookup_backward->exists(sn->extract_item(i)->get_name()))
			local_name = lookup_backward->extract_txt(sn->extract_item(i)->get_name());
		else
			continue;

		switch(sn->extract_item(i)->get_type())
		{
			case SInt: 
				scomposite->append(pack(sn->extract_int(i), local_name));
				children++;
				break;
			case SDouble:
				scomposite->append(pack(sn->extract_dbl(i), local_name));
				children++;
				break;
			case SText:
				scomposite->append(pack(sn->extract_txt(i), local_name));
				children++;
				break;
			case SBinary:
				scomposite->append(pack(sn->extract_bin(i), sn->num_bytes(i), local_name));
				children++;
				break;
			case SBool: 
				scomposite->append(pack_bool(sn->extract_flg(i), local_name));
				children++;
				break;
			case SDateTime:
				scomposite->append(pack(sn->extract_clk(i), local_name));
				children++;
				break;
			case SLocation:
				scomposite->append(pack(sn->extract_loc(i), local_name));
				children++;
				break;
			case SStruct:
				child = repack(sn->extract_item(i));
				
				if (child != NULL)
				{
					// Find this structure in the hash lookup.
					top = owner->msg_schema->hashes->find(scomposite->get_name());
					// Iterate through the children (-2 to skip "has" and "similar").
					for (; children < top->count() - 2; children++)
					{
						// If the child has the same name as our node, pack it, otherwise pack an empty node.
						if (!strcmp(top->extract_item(children)->get_name(), child->get_name()))
						{
							scomposite->append(child);
							break;
						}
						else
							scomposite->append(pack((const char *)NULL, top->extract_item(children)->get_name()));
					}
				}	
				
				break;
			case SList:
				child = repack(sn->extract_item(i));
				if (child != NULL)
				{
					scomposite->append(child);
					children++;
				}
				break;
			case SValue:
				scomposite->append(pack(sn->extract_value(i), local_name));
				children++;
				break;
			case SEmpty: ; break;
			default:
				error("Unknown node type ");
				break;
		}
	}
	return scomposite;
}

void speer::serve(snode *sn, int seq, HashCode *hc)
{
	// return to library smessage: MessageRcv
	smessage *msg;

	msg = new smessage();
	msg->type = MessageRcv;
	msg->source_cpt = sdup(cpt_name);
	msg->source_inst = sdup(instance);
	msg->source_ep = sdup(endpoint);
	msg->source_ep_id = ep_id;
	msg->seq = seq;
	msg->hc = new HashCode(hc);
	// We use actual msg hash, not owner->msg_hc, as this might be polymorphic
	msg->tree = sn; // Consumes sn
	
	// store details in one of pending_replies
	scomm *later;
	later = new scomm();
	later->source = sdup(*(owner->cpt_name));
	later->src_endpoint = sdup(owner->name);
	later->target = sdup(cpt_name);
	later->tgt_endpoint = sdup(endpoint);
	later->type = MessageClient;
	later->seq = seq;
	later->hc = new HashCode(owner->reply_hc); // May be overriden later if poly
	later->peer_uid = uid;

	/* We only need to fill in int later->length and uchar *later->data
		with the answer from the library, then it can be sent back to the
		client */
	owner->pending_replies->add(later);
	
	owner->deliver_local(msg);
	delete msg;
}

void speer::client(snode *sn, int seq, HashCode *hc)
{
	// return to library smessage: MessageResponse
	// consume one of issued_rpcs
	smessage *msg;

	msg = owner->issued_rpcs->remove();
	if(msg == NULL)
		error("RPC reply received from server, but request has gone missing");
	if(msg->seq != seq)
		error("RPC reply sequence number mismatch");
	msg->tree = sn; // Consumes sn
	msg->hc = new HashCode(hc);
	// We use actual msg hash, not owner->reply_hc, as this might be polymorphic
	owner->deliver_local(msg);
	delete msg;
}

speer::speer()
{
	cpt_name = instance = endpoint = address = NULL;
	subs = NULL;
	topic = NULL;
	disposable = 0;
	msg_poly = reply_poly = 0;
	ep_id = 0; // Needs to be filled in
	map_constraint = NULL;
	container = NULL;
	layer = NULL;
	lookup_forward = lookup_backward = NULL;
}

speer::~speer()
{
	if(cpt_name != NULL) delete[] cpt_name;
	if(instance != NULL) delete[] instance;
	if(endpoint != NULL) delete[] endpoint;
	if(address != NULL) delete[] address;
	if(subs != NULL) delete subs;
	if(topic != NULL) delete[] topic;
	if(map_constraint != NULL) delete[] map_constraint;
	if(container != NULL) delete container;
	if(layer != NULL) delete layer;
	if(lookup_forward != NULL) delete lookup_forward;
	if(lookup_backward != NULL) delete lookup_backward;
}

void speer::divert(const char *new_address, const char *new_endpoint)
{
	sdivert *div;
	AbstractMessage *abst;
	AbstractMessageQueue *q;
	
	// Form divert message:
	div = new sdivert();
	div->src_cpt = sdup(*(owner->cpt_name));
	div->src_ep = sdup(owner->name);
	div->tgt_cpt = sdup(cpt_name);
	div->tgt_ep = sdup(endpoint);
	div->new_cpt = sdup(new_address);
	div->new_ep = sdup(new_endpoint);
	
	// Preselect to send divert message:
	abst = div->wrap(sock);
	delete div;
	abst->diverting = 1;
	q = wrap->progress_out[sock];

	if(!q->isempty()) // Already a message being sent
		q->add(abst);
	else
	{
		wrap->preselect_fd->add(sock);
		wrap->preselect_direction->add(FDOutbound);
		wrap->multi->add(sock, MULTI_WRITE, "speer::divert");
		q->add(abst);
	}	
	// When complete, continue_write() will call departure(peer) for us
}

void speer::resubscribe(const char *subs, const char *topic)
{
	sresub *resub;
	AbstractMessage *abst;
	AbstractMessageQueue *q;
	
	// Form resub message:
	resub = new sresub();
	resub->src_cpt = sdup(*(owner->cpt_name));
	resub->src_ep = sdup(owner->name);
	resub->tgt_cpt = sdup(cpt_name);
	resub->tgt_ep = sdup(endpoint);

	//test...
	if (owner->flexible_matching && this->lookup_forward != NULL)
	{
		// If we support flexible matching, and there's a lookup table for this (hence they're different schemas)
		// Convert the subscription string to peer's schema before sending.
		subscription *s = new subscription(subs);
		resub->subscription = sdup(s->dump_plaintext(this->lookup_forward));
	}
	else
	{	
		resub->subscription = sdup(subs);
	}
	
	resub->topic = sdup(topic);

	// Preselect to send resub message:
	abst = resub->wrap(sock);
	delete resub;
	q = wrap->progress_out[sock];
	if(!q->isempty()) // Already a message being sent
		q->add(abst);
	else
	{
		wrap->preselect_fd->add(sock);
		wrap->preselect_direction->add(FDOutbound);
		wrap->multi->add(sock, MULTI_WRITE, "speer::resubscribe");
		q->add(abst);
	}

}



/* SchemaCache */

typedef SchemaVector *SchemaVectorPtr;

SchemaCache::SchemaCache()
{
	tablesize = 97;
	all = new SchemaVector();
	hashtable = new SchemaVectorPtr[tablesize];
	for(int i = 0; i < tablesize; i++)
		hashtable[i] = NULL;
}

SchemaCache::~SchemaCache()
{
	for(int i = 0; i < tablesize; i++)
		if(hashtable[i] != NULL)
			delete hashtable[i];
	delete[] hashtable;
	
	for(int i = 0; i < all->count(); i++)
		delete all->item(i);
	delete all;
}

void SchemaCache::expand()
{
	int slot;
	SchemaVector *v;
	Schema *sch;
	
	for(int i = 0; i < tablesize; i++)
		if(hashtable[i] != NULL)
			delete hashtable[i];
	delete[] hashtable;
	
	if(tablesize < 100) tablesize = 997;
	else if(tablesize < 1000) tablesize = 9973;
	else if(tablesize < 10000) tablesize = 99991;
	else error("Schema cache too large");
	
	hashtable = new SchemaVectorPtr[tablesize];
	for(int i = 0; i < tablesize; i++)
		hashtable[i] = NULL;
	
	// Reinsert items:
	for(int i = 0; i < all->count(); i++)
	{
		sch = all->item(i);
		slot = sch->hc->toint() % tablesize;
		v = hashtable[slot];
		if(v == NULL)
		{
			v = new SchemaVector();
			hashtable[slot] = v;
		}
		v->add(sch);
	}
}

Schema *SchemaCache::add(Schema *sch, int *already_known)
{
	Schema *copy;
	int slot;
	SchemaVector *v;
	Schema *test;

	char *s = sch->hc->tostring();
	slot = sch->hc->toint() % tablesize;
	/* printf("SchemaCache::add(%s), num %d, slot %d, tablesize %d\n", s,
			sch->hc->toint(), slot, tablesize); */
	delete[] s;
	v = hashtable[slot];
	if(v == NULL)
	{
		v = new SchemaVector();
		hashtable[slot] = v;
	}
	for(int i = 0; i < v->count(); i++)
	{
		test = v->item(i);
		if(test->hc->equals(sch->hc))
		{
			if(already_known != NULL)
				*already_known = 1;
			return test; // Already present
		}
	}
	copy = new Schema(sch);
	v->add(copy);
	all->add(copy);
	if(all->count() * 4 > tablesize) // Hash table more than 25% full
		expand();
	if(already_known != NULL)
		*already_known = 0;
	return copy;
}

Schema *SchemaCache::lookup(HashCode *hc)
{
	int slot;
	SchemaVector *v;
	Schema *sch;
	
	slot = hc->toint() % tablesize;
	v = hashtable[slot];
	if(v == NULL)
		return NULL;
	for(int i = 0; i < v->count(); i++)
	{
		sch = v->item(i);
		if(sch->hc->equals(hc))
			return sch;
	}
	return NULL;
}

void SchemaCache::dump()
{
	int n = all->count();
	Schema *sch;

	printf("Dumping %d\n", n);
	for(int i = 0; i < n; i++)
	{
		printf("Schema %d:\n", i);
		sch = all->item(i);
		sch->dump_tree(3);
	}
}
