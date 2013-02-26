// rdc.h - DMI - 23-6-2007

class imagevector;

class persistence
{
	public:
	
	persistence();
	~persistence();
	
	linefile *cpts;
	svector *procs;
	char *sbus_dir;
	
	void start(const char *cmd);
	void start_all();
	int is_persistent(const char *cmd);
	void hup(sendpoint *ep, imagevector *live, pthread_mutex_t *live_mutex,
			sendpoint *terminate_ep);
	
	private:
			
	svector *running_procs();
	int is_running(const char *proc);
	
	void detach();
	
	// Utilities:
	int is_an_integer(const char *name);
	void checkquotes(const char *cmd);
	svector *split(const char *s);
};


class image
{
	public:

	image();
	void init_hashes();
	~image();
				
	int match(snode *interface, snode *constraints, scomponent *com, const char *principal_cpt, const char *principal_inst);
	int hashmatch(const char *hsh1, const char *hsh2);
	int similar(const char *name, const char *instance_name, const char *addr);
	
	char *address;
	char *cpt_name;
	char *ins_name;
	snode *metadata, *state;
	svector *msg_hsh, *msg_type_hsh, *reply_hsh, *reply_type_hsh;
	
	int local, persistent;
	int lost;
	
	struct timeval tv_ping;

	/*void add_permission(const char *endpt, const char *principal_cpt, const char *principal_inst);
	void remove_permission(const char *endpt, const char *principal_cpt, const char *principal_inst);
	int authorised(const char *endpt, const char *principal_cpt, const char *principal_inst);*/

	//Vector the all the access control policies for the instance
    rdcpermissionstore *acpolicies;
    snodevector *buffered_policies;
    
    // a mutex so that we only check the component is still alive in one thread at a time.
   	pthread_mutex_t checkalive_mutex;
   	
   	private:
   	
	int match_cpt_metadata(snode *constraints);
};



class imagevector : public pvector // wrapper class
{
	public:
			
		void add(image *x) { pvector::add((void *)x); }
		void set(int n, image *x) { pvector::set(n, (void *)x); }		
		image *item(int n) { return (image *)pvector::item(n); }
		void remove(image *x) { pvector::remove((void *)x); }
};

class rdc
{
	public:
			
	rdc();
	~rdc();
	
	void mainloop();
	void join();
	
	void registercpt();
	void lost();
	void deregister(const char *addr);
	void lookup(sendpoint *ep);
	void list(sendpoint *ep);
	void dump(sendpoint *ep);
	void remotestart(sendpoint *ep);
	void schedule_check();


	int is_local(const char *address);


	scomponent *com;
	
	sendpoint *events_ep, *register_ep, *lost_ep;
	sendpoint *dump_client_ep;
	sendpoint *metadata_ep, *status_ep;
	sendpoint *terminate_ep;
	sendpoint *acl_ep; //the end point to handle ACL msgs.

	sendpointvector *metadata_epv, *status_epv;
	intvector *pool_busy;
	imagevector *live;
	imagevector *registering; //keeps track of 'registering' images...
	
	/* Protects the "live" data structure (but not the contents of
		individual images, so there is a possible concurrency issue
		with modifications to them): */
	pthread_mutex_t live_mutex;
	// Protects pool_busy, which in turn guards metadata_epv and status_epv:
	pthread_mutex_t pool_mutex;
	pthread_mutex_t events_mutex; // Arbitrates use of events_ep->emit()
	pthread_mutex_t registering_mutex;

	pthread_attr_t attr;
	
	persistence *persist;

	//rdcpermissionstore *permissions;

	void process_permission_change(snode *permissionnode); //actually change the ACL

	private:
	svector *builtin_names;
	
	void checkbuiltins();
	void read_local_address();
	
	void load_permissions(const char *filename); //load permissions from a file (defaults) -- NB this is different from the component/wrapper cpt, as RDC privileges can refer to OTHER componetns...
	void change_permissions(sendpoint *ep); //change the ACL in response to a message.

	char *local_address;
	
	// Only call this from the master thread, since it may call clone():
	int first_free_pool();
	
	struct timeval tv_start;


};

void release_pool(int pool_id, pthread_mutex_t *pool_mutex_ptr,
		intvector *pool_busy);

struct threadarg
{
	rdc *obj;
	int pool_id;
	image *img;
};

// checkalive() could block in status_ep rpc, so separate threads run it:
void *checkalive(void *arg);
/* registerback() could block in status_ep or metadata_ep, so separate
	threads run it: */
void *registerback(void *arg);



