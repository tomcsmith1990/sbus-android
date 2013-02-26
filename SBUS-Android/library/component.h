// component.h - DMI - 21-3-07

class sadmin;
class smessage;
class scomponent;
class sendpoint;
class sbuiltin;
class MapConstraints;

enum EndpointType
{ EndpointServer, EndpointClient, EndpointSource, EndpointSink };

extern const int NUM_ENDPOINT_TYPES;

// Array to convert from enumeration to values:
extern const char *endpoint_type[];

enum CptUniqueness { UniqueMultiple, UniqueSingle, UniqueReplace };

typedef const char *constcharptr;

class scomponent
{
	public:
	
	scomponent(const char *cpt_name, const char *instance_name = NULL,
			CptUniqueness unique = UniqueMultiple);
	~scomponent();
		
	sendpoint *add_endpoint(const char *name, EndpointType type,
			const char *msg_hash, const char *reply_hash = NULL);
	sendpoint *add_endpoint(const char *name, EndpointType type,
			HashCode *msg_hc, HashCode *reply_hc = NULL);
	void add_rdc(const char *address);
	void remove_rdc(const char *address);
	void start(const char *metadata_filename, int port = -1, int register_with_rdc = true);
	void stop();
	sendpoint *clone(sendpoint *ep); // Convenience method, wraps add_endpoint()
	
	int count_endpoints();
	sendpoint *get_endpoint(int index);
	sendpoint *get_endpoint(const char *name);
	sendpoint *fd_to_endpoint(int fd);
	
	void set_rdc_update_notify(int notify);
	void set_rdc_update_autoconnect(int autoconnect);
	
	static void set_log_level(int log, int echo);
		
	// The following are valid after start() has returned:
	int count_builtins();
	sbuiltin *get_builtin(int index); // No need to delete sbuiltin
	svector *get_builtin_names(); //returns whether an endpoint name is a builtin name
	int is_builtin(const char *epname);
	int get_listen_port();
	const char *get_address();        // No need to delete string
	snode *get_status();              // Caller should delete snode after use
	const char *get_schema(HashCode *hc); // Caller should delete string
	
	sendpoint *rdc_update_notifications_endpoint();

	HashCode *declare_schema(const char *schema);
	HashCode *load_schema(const char *file);
	
	void load_permissions(const char *filename); //this loads privileges from the file.
	void set_permission(const char *pr_cpt, const char *pr_inst, int authorised); //sets a permission on every endpoint of a component..
	void set_permission(const char *target_ep, const char *pr_cpt, const char *pr_inst, int authorised); //sets the permission for a specific endpoint (wrapper for the sendpoint setperm)

	private:

	char *name, *instance;
	CptUniqueness uniq;
	
	int callback_fd; // Listens for new connections from the wrapper						
	int bootstrap_fd; // First connection, creates endpoint pipes

	int listen_port;
	const char *canonical_address;
	
	int rdc_update_notify;
	int rdc_update_autoconnect;
		
	void start_wrapper();
	void running();
	
	void update_rdc_settings(const char *address = NULL, int arrived = -1);
	
	int wrapper_started;

	HashCode *do_declare_schema(const char *schema, int file_lookup);
	sendpoint *do_add_endpoint(const char *name, EndpointType type,
			HashCode *msg_hc, HashCode *reply_hc);
	

	sbuiltinvector *biv;
	sendpointvector *epv;
	svector *rdc;
};

class sbuiltin
{
	public:
	
	sbuiltin();
	~sbuiltin();
	
	char *name;
	EndpointType type;
	HashCode *msg_hc, *reply_hc;
};

class sendpoint
{
	public:
	
	sendpoint(const char *name, EndpointType type,
		HashCode *msg_hc, HashCode *reply_hc);
	~sendpoint();

	/* map() returns a string containing the address of the
		other component if successful, otherwise NULL.
		The caller must delete this string in the successful case.
		If endpoint is NULL, assume same name as this end: */
	char *map(const char *address, const char *endpoint, int sflags = 0,
			const char *pub_key = NULL);

	/**Unmaps the endpoint. Params specify the specific component/endpoint. NULL parameters apply to all.
	 *
 	 * NOTE: the address is compared by value!
	 * Addresses should be specified by the full IP:PORT of the peer 
	 * (from the HELLO/WELCOME) as stored by the component (or RDC) 
	 * */
	void unmap(const char *address, const char *endpoint);
	//wrapper to disconnect all connections on the endpoint (old functionality, calls unmap(NULL,NULL)
	void unmap();
	
	void set_automap_policy(const char *address, const char *endpoint);

	int ismapped();
	
	char *map(MapConstraints *constraints, int sflags); // Unimplemented
	


	void set_permission(const char *pr_cpt, const char *pr_inst, int authorised,  int apply_to_all_eps = 0);
	void load_permissions(const char *filename); //this loads privileges from the file.


	void subscribe(const char *topic, const char *subs, const char *peer);
	/* "peer" may be a component or instance name, or "*" for all.
		If it is NULL, no currently mapped peer is resubscribed but the
		default subscription to use for future maps is set. */
	
	/* client calls rpc(), server calls rcv() & reply(),
		source calls emit(), sink calls rcv(): */
	smessage *rcv();
	void reply(smessage *query, snode *result, int exception = 0,
			HashCode *hc = NULL);
	void emit(snode *msg, const char *topic = NULL, HashCode *hc = NULL);
	smessage *rpc(snode *query, HashCode *hc = NULL);
	
	int message_waiting(); // Returns the number of pre-fetched messages
	
	char *name;
	EndpointType type;
	
	int fd; // Used by the library to talk to the wrapper

	HashCode *msg_hc, *reply_hc; // Specified when defined; type checks
	

	private:
			
	smessage *read_returncode();
	
	smessagequeue *postponed;
};

enum TiePolicy { TieRandom, TieLoad };
enum MatchMode { MatchPartial, MatchExact, MatchContained };

class MapConstraints
{
	public:
	
	MapConstraints();
	MapConstraints(const char *string);
	~MapConstraints();
	
	/* is_constraint returns 1 if the string looks like a constraint string,
		0 if it looks like host and/or port, and -1 if it resembles
		neither of these (and hence is definitely an invalid address).
		Note: the string may still fail to pass more detailed parsing checks
		in cases 0 or 1: */
	static int is_constraint(const char *string);
	
	snode *pack();

	void set_name(const char *s);	
	void set_instance(const char *s);	
	void set_creator(const char *s);	
	void set_key(const char *s);	
	
	void add_keyword(const char *s);
	void add_peer(const char *s);
	void add_ancestor(const char *s);
				
	int match_endpoint_names;

	// Note: the next two fields are currently unsupported
	MatchMode match_mode;
	TiePolicy tie_policy; // Used if several components match
	
	int failed_parse;
	
	private:
			
	void init();
	char *read_word(constcharptr *string);
			
	svector *keywords;
	svector *peers, *ancestors;
	
	// Any of these may be NULL to indicate irrelevant:
	const char *cpt_name;
	const char *instance_name;
	const char *creator;
	const char *pub_key;
	const char *hash;
	const char *type_hash;	
};

// Values for sflags:
const int UnavailableSilent = 1 << 0;
const int DisconnectSilent = 1 << 1;
const int AttemptFailover = 1 << 2;
const int AttemptReconnect = 1 << 3;
const int AllowMigration = 1 << 4;
const int AwaitTransfer = 1 << 5;
