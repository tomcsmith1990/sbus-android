// wrapper.h - DMI - 12-5-07
class scontrol;
class sadmin;
class saddendpoint;
class smidpoint;
class shello;
class sgoodbye;
class sdivert;
class swrapper;
class SchemaCache;
class AbstractMessage;



enum FDState
{
	FDUnused,     // Not open
	FDListen,     // External master socket only
	FDBoot,       // Bootstrap socket only
	FDLibrary,    // Local connection to library
	FDPeer,       // Normal remote connection to peer
	// Accepting new incoming connections:
	FDAccepted,   // Awaiting hello or visitor message
	FDWelcoming,  // Sending welcome message (which may be rejection, of course)
	// Mapping new outgoing connections:
	FDMapping,    // Waiting for connect to finish, before sending hello msg
	FDGreeting,   // Sending hello message
	FDPending,    // Waiting for approval (or not) from a welcome message
	// Disposable connections:
	FDConnecting, // Waiting for connect to finish, before sending visitor msg
	FDVisiting,   // Sending visitor message
	FDDisposing   // Awaiting reply to visitor message
};

enum FDDirection
{
	FDOutbound, // Currently trying to send a message on this fd
	FDInbound,  // Currently waiting to receive a message on this fd
	FDUnbound   // connect() has not yet returned (FDMapping or FDConnecting)
};

const char *address_wildcard = "*:*"; //hack: address to mean apply unmap/divert to "all addresses". bypasses mapconstraint tests..




class speer
{
	public:
	
	speer();
	~speer();
	
	int sock;
	int uid;
	int disposable;

	char *cpt_name; // Remote side
	char *instance; // Remote side
	char *endpoint; // Remote side
	char *address;  // Remote side (canonical)
	int ep_id;      // Remote side

	subscription *subs;	
	char *topic;
	int msg_poly, reply_poly;

	void sink(snode *sn, HashCode *hc, const char *topic);
	void serve(snode *sn, int seq, HashCode *hc);
	void client(snode *sn, int seq, HashCode *hc);
	
	void divert(const char *new_address, const char *new_endpoint);
	void resubscribe(const char *subs, const char *topic);

	smidpoint *owner;
	
	/* Other information describing our peer component will go here...
		E.g. metadata */
	
	void deliver_remote(scomm *msg, int disrupt = 0);
};

class smidpoint
{
	public:
	
	smidpoint();
	~smidpoint();
	
	char **cpt_name;
	char *name;
	EndpointType type;
	int next_seq;
	int ep_id;
			
	speervector *peers;
	int fd; // Used to talk to the library
	int builtin; // Flag
	
	char *subs;
	char *topic;

	// Performance monitoring:
	int processed;
	int dropped;
		
	smessagequeue *issued_rpcs; // EndpointClient only
	scommqueue *pending_replies; // EndpointServer only
	
	char *verify_metadata(snode *sn);
	int compatible(const char *required_endpoint, int required_clone,
			EndpointType type, HashCode *msg_hc, HashCode *reply_hc);
	snode *pack_interface(int invert);
	
	void emit(const char *topic, const char *xml, HashCode *hc);
	void emit(const char *topic, snode *sn, HashCode *hc); // Internal
	void rpc(const char *xml, HashCode *hc);
	void reply(int seq, const char *xml, HashCode *hc);
	
	void deliver_local(smessage *msg);

	Schema *msg_schema, *reply_schema; // Read from metadata
	HashCode *msg_hc, *reply_hc; // Specified when defined, for type checking


	//ACL for the endpoint
	spermissionvector *acl_ep;

};

struct registerparams
{
	int arrive; // 1 = register, 0 = deregister
	int count;  // Number of RDCs to try
	int failed, succeeded; // When failed + succeeded = count, all accounted for
};

enum ResolveFunction{
	RSMap,
	RSUnmap,
	RSDivert
};

class mapparams
{
	public:
	
	mapparams();
	~mapparams();
	
	smidpoint *mp;
	snode *query_sn;
	const char *endpoint;
	int report_fd;
	
	svector *possibilities; // Target component addresses suggested
	svector *local_possibilities; // Target component addresses suggested (from a local search of peers)
	int remaining_rdcs; // RDCs which haven't responded (or failed) yet
	ResolveFunction operation; //the operation that the mapparams is resolving.
	
	//extra params to handle diverts
	const char *newaddr;
	const char *newendpt;
};



class swrapper
{
	public:
			
	swrapper(const char *callback_address);
	~swrapper();
	
	void bootstrap();
	void run();
	void describe();
	
	AbstractMessage **progress_in; // inbound messages
	AbstractMessage **progress_con; // connections in progress
	AbstractMessageQueue **progress_out; // outbound messages
	FDState *fdstate;
	
	intqueue *preselect_fd; // Process this before select()
	intqueue *preselect_direction; // Holds FDDirection's (inbound/outbound/con)
	
	multiplex *multi;
	
	SchemaCache *cache;

	void departure(speer *peer, int unexpected);
	
	private:
	
	void begin_accept();
	void do_accept(int dyn_sock, AbstractMessage *abst);
	void finalise_accept(int dyn_sock, speer *peer);
	const char *verify_metadata(snode *metadata);
	void set_admin(snode *metadata);
	smidpoint *add_endpoint(saddendpoint *add);
	void running(int listen_port);
	char *get_cmdline(int pid);
	
	void change_privileges(privilegeparams *params);
	void update_privileges_on_rdc(privilegeparams *params, const char *address = NULL);
	void load_privileges_from_file(const char *filenm);

	void add_builtin_endpoints();
	smidpoint *add_builtin(const char *endpoint, EndpointType type,
			const char *msg_hash, const char *reply_hash = NULL);
	void verify_builtin(smidpoint *mp);
	snode *pack_metadata();
	snode *pack_status();
	snode *pack_privileges();
	void serve_boot();
	void serve_peer(AbstractMessage *abst, speer *peer);
	void serve_peer(scomm *msg, speer *peer);
	void serve_endpoint(AbstractMessage *abst, smidpoint *ep);
	void serve_builtin(speer *peer, snode *sn, int seq);
	void serve_sink_builtin(const char *fn_endpoint, snode *sn);
	void serve_rpc_builtin(speer *peer, snode *sn, int seq);
	void serve_goodbye(speer *peer, sgoodbye *oob_goodbye);
	void serve_resub(speer *peer, sresub *oob_resub);
	void serve_divert(speer *peer, sdivert *oob_divert);
	void serve_visitor(int fd, shello *hello);

	void identify_fd(int fd, smidpoint **mp, speer **peer);
	void continue_read(int fd, smidpoint *mp, speer *peer);
	void continue_write(int fd, speer *peer);
	void continue_accept(int fd);
	void continue_welcome(int fd);
	void continue_greeting(int fd);
	void continue_approval(int fd);
	void continue_visit(int fd);
	void continue_dispose(int fd);
	void continue_mapping(int fd);
	void continue_connect(int fd);
	
	void register_cpt(int arrive, const char *address = NULL);
	void handle_new_rdc(int arrive, const char *address);
	void check_dirn(int dirn, int expected, const char *fd_type);
	void lost(speer *peer);
	

	smidpoint *register_mp, *lookup_cpt_mp, *lookup_schema_mp, *lost_mp, *rdcacl_mp, *dump_privilege_mp, *rdc_update_mp;
	
	int bootstrap_fd;
	int external_master_sock;
	int listen_port;
	int register_with_rdc; //should we register with the rdc.
	int rdc_update_notify; // should we send a callback message when we get a new rdc message.
	int rdc_update_autoconnect;
	CptUniqueness uniq;
	char *canonical_address;
	char *local_address;
	
	char *cpt_name, *instance_name;
	char *cmdline;
	sadmin *admin;
	svector *rdc;
		
	smidpointvector *mps;
	int next_uid, next_visit;		

	smidpoint *get_status_mp;


	// report_fd is -1 if no report to be sent in map() and unmap()
	void map(smidpoint *mp, const char *addrstring, const char *endpoint,
			int report_fd);
	void do_map(mapparams *params);
	void finalise_map(int fd, AbstractMessage *abst);
	void unmap(smidpoint *mp, const char *addrstring, const char *endpoint, int report_fd);
	void do_unmap(mapparams *params);
	void do_divert(mapparams *params);
	void ismap(smidpoint *mp, int report_fd);
	void subscribe(smidpoint *mp, const char *subs, const char *topic,
			const char *peer_address);
	Schema *declare_schema(const char *schema, int file_lookup);
	void resolve_address(const char *addrstring, mapparams *params);
	void resolve_address_local(smidpoint *mp, const char *addrstring, mapparams *params);
	void continue_resolve(mapparams *params);
	void map_report(int report_fd, int code, const char *address = NULL);
	snode *atomic_visit(const char *addr, const char *cpt,
			const char *ep, smidpoint *mp, snode *sn);
	int begin_visit(VisitPurpose purpose, const char *addr, const char *cpt,
			const char *ep, smidpoint *mp, snode *sn,
			mapparams *map_params, registerparams *reg_params,
			scomm *unrecognised);
	void finalise_server_visit(AbstractMessage *abst);
	void finalise_sink_visit(AbstractMessage *abst);
	void abort_visit(AbstractMessage *abst);
	void setdefaultprivs();
};

class sadmin
{
	public:
	
	const char *description;
	svector *keywords;
	const char *designer;
	const char *creator;
};

class SchemaCache
{
	public:
	
	SchemaCache();
	~SchemaCache();

	// Returns the copy of the schema in the cache:
	Schema *add(Schema *sch, int *already_known = NULL);
	Schema *lookup(HashCode *hc); // NULL if not found
	
	void dump();
	
	private:
			
	void expand();

	SchemaVector *all;
	SchemaVector **hashtable;
	int tablesize;
};
