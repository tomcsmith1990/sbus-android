// net.h - DMI - 14-4-2007

const int PROTOCOL_VERSION = 8;

extern const int default_rdc_port;

class mapparams;
class registerparams;

enum MessageType
{
	// Component to component:
	MessageHello, MessageWelcome, // Remote connection setup
	MessageSink, MessageServer, MessageClient, // Remote data transfer
	MessageFlow, // Flow control, sent from sink to source
	MessageGoodbye, MessageResubscribe, MessageDivert, // Out-of-band messages
	MessageVisitor, // Disposable connections
	
	// Library to wrapper:
	MessageAddEndpoint, MessageStart, // Bootstrap pipe setup
	MessageStop, // Bootstrap pipe runtime/close
	MessageRunning, // Bootstrap pipe reply
	MessageGetStatus, MessageStatus, // Bootstrap pipe runtime
	MessageGetSchema, MessageSchema, // Bootstrap pipe runtime
	MessageDeclare, MessageHash,     // Bootstrap pipe runtime
	MessageMap, MessageUnmap, MessageIsmap, // Local mapping control messages
	MessageRdc,
	MessageReturnCode, // Mapping control reply
	MessageSubscribe,  // Local control message
	MessageEmit, MessageRPC, MessageReply, // Library to wrapper
	MessageRcv, MessageResponse, MessageUnavailable, // Wrapper to library
		
	MessagePrivilege, //for access control privileges (add/rem)
	MessageLoadPrivileges, //
	MessageMapPolicy,
	MessageUnknown // Internal use
};

enum AcceptanceCode // Must match acceptance_code in net.cpp
{
	AcceptOK,
	AcceptWrongCpt,
	AcceptNoEndpoint,
	AcceptWrongType,
	AcceptNotCompatible,
	AcceptNoAccess,
	AcceptProtocolError,
	AcceptAlreadyMapped
};
// Array to convert from enumeration to values:
extern const char *acceptance_code[];

enum MessageState { StateHeader, StateBody, StateDone };

enum VisitPurpose
{
	NonVisit,
	VisitRegister,
	VisitLookupSchema,
	VisitResolveConstraints,
	VisitLost,
	VisitUpdatePrivilege
};
	
class AbstractMessage
{
	public:
	
	AbstractMessage(int fd, StringBuf *sb); // Outbound (sb has type in header)
	AbstractMessage(int fd); // Inbound
	~AbstractMessage();

	int advance(); /* Returns 1 if complete, 0 if incomplete, -1 on disconnect
	               or -2 on protocol error (latter for inbound messages only) */
	int blockadvance(); // Like advance(), but never returns 0 and may block
	int blockadvance(int timeout_usec); // As above, but returns 0 if timeout
	int partial_advance(); // For testing purposes on writes only

	unsigned char *get_data();
	int get_length(); // length of data (body)
	MessageType get_type();
	
	/* Protocol-specific state needed to complete an operation; carried along
		for the ride in AbstractMessage (not our responsibility to free): */
	speer *peer;           // FDWelcoming, FDGreeting and FDPending
	int report_fd;         // FDGreeting and FDPending
	int diverting;         // FDPeer

	/* Protocol-specific state needed for internal visitor connections: */
		
	VisitPurpose purpose;  // Visitor connections only, else NonVisit
	Schema *reply_schema;  // FDVisiting and FDDisposing
	mapparams *map_params; // FDVisiting and FDDisposing (resolving constraints)
	registerparams *reg_params; // Likewise (registering or deregistering)
	scomm *unrecognised;   // FDVisiting and FDDisposing (schema lookups)
	int terminate;         // FDVisiting
	char *address;         // FDConnecting
	
	/* End of protocol-specific passenger state */
		
	private:

	int do_advance(int block, int timeout_usec, int max_bytes = -1);
	int ready(int timeout_usec = 0);
	// Waits for fd to become ready; returns -1 on timeout, else 0
				
	unsigned char *header; // Only considered separately for inbound
	unsigned char *body;
	int length; // Total length (header + body)
	MessageType type;
	
	int fd;
	int outbound; // flag
	int filled; // bytes processed in this phase (header/body) so far
	MessageState state;
};

/* The various protocol message classes are containers for the data
	transmitted by that message type. They are converted to AbstractMessages
	with their wrap() method (ready for transmission with advance()).
	An incoming AbstractMessage (which has been received using advance())
	is narrowed to a particular protocol message type class using the
	reveal() method of that class.
	wrap() and reveal() are therefore the equivalents of read() and write()
	but using AbstractMessages instead of acting directly on file
	descriptors. */

class sproto
{
	public:
			
	int read(int sock); // -1 on disconnect, -2 on bad protocol, else 0
	int write(int sock); // -1 on disconnect, else 0
	virtual int reveal(AbstractMessage *abst) = 0; // -1 wrong msg type, else 0
		// reveal() may also return -2 for bad XML! - XXXX
	virtual AbstractMessage *wrap(int fd) = 0;
};

class scontrol : public sproto
{
	public:

	scontrol();
	~scontrol();

	int reveal(AbstractMessage *abst); // returns -1 if wrong msg type, else 0
	AbstractMessage *wrap(int fd);
				
	MessageType type; // MessageMap, MessageUnmap, MessageIsmap
	                  // or MessageSubscribe
	
	// For the three mapping calls:
	char *address;
	char *target_endpoint;
	
	// For MessageSubscribe:
	char *subs;
	char *topic;
	char *peer;

	//For the permissions
	char *principal_cpt;
	char *principal_inst;
	int adding_permission;
	//for loading permissions from a file...
	char *filename;

};

class srdc : public sproto
{
	public:

	srdc();
	~srdc();

	int reveal(AbstractMessage *abst); // returns -1 if wrong msg type, else 0
	AbstractMessage *wrap(int fd);
				
	MessageType type; // MessageRdc
	
	// For the three mapping calls:
	char *address;
	int arrived;
	int notify;
	int autoconnect;
};

class srunning : public sproto
{
	public:

	srunning();
	~srunning();
		
	int reveal(AbstractMessage *abst); // returns -1 if wrong msg type, else 0
	AbstractMessage *wrap(int fd);
	
	int listen_port;
	char *address;
	snode *builtins;
	
	private:
			
	char *xml; // Managed internally by srunning - callers just set/get builtins
};

class sreturncode : public sproto
{
	public:
	
	sreturncode();
	~sreturncode();
			
	int reveal(AbstractMessage *abst); // returns -1 if wrong msg type, else 0
	AbstractMessage *wrap(int fd);
	
	int retcode;
	char *address;
};

class sstopwrapper : public sproto
{
	public:
	
	int reveal(AbstractMessage *abst); // returns -1 if wrong msg type, else 0
	AbstractMessage *wrap(int fd);
	
	int reason;
};

class shook : public sproto
{
	public:
	
	shook(MessageType t);
	shook();
	~shook();
			
	int reveal(AbstractMessage *abst); // returns -1 if wrong msg type, else 0
	AbstractMessage *wrap(int fd);
	
	MessageType type; // MessageGetStatus, MessageGetSchema, MessageDeclare
	
	HashCode *hc;	
	snode *tree;
	
	/* MessageGetStatus: 0         - 000000000000
		MessageGetSchema: @txt hash - D3C74D1897A3
		MessageDeclare: @declare { txt schema flg file_lookup } - 3D79D04FEBCC */
};

class sgeneric : public sproto
{
	public:
	
	sgeneric(MessageType t);
	sgeneric();
	~sgeneric();
			
	int reveal(AbstractMessage *abst); // returns -1 if wrong msg type, else 0
	AbstractMessage *wrap(int fd);

	MessageType type; // MessageStatus, MessageSchema, MessageHash
	
	HashCode *hc;	
	snode *tree;
	
	/* MessageStatus: ^component-state
		MessageSchema: @txt schema - D39E44946A6C
		MessageHash:   @txt hash - D3C74D1897A3 */
};

class saddendpoint // MessageAddEndpoint
{
	public:	
			
	saddendpoint();
	~saddendpoint();
	void clear();
	
	int write(int sock); // -1 if disconnected, else 0
	AbstractMessage *wrap(int fd);
	
	char *endpoint;
	EndpointType type;
	HashCode *msg_hc;
	HashCode *reply_hc;
	int partial_matching;
};

class sstartwrapper
{
	public:

	sstartwrapper();
	~sstartwrapper();
						
	int write(int sock); // -1 if disconnected, else 0
	AbstractMessage *wrap(int fd);

	char *cpt_name;
	char *instance_name;
	char *creator;
	char *metadata_address;
	int listen_port;
	CptUniqueness unique;
	int log_level;
	int echo_level;
	int rdc_register;
	int rdc_update_notify;
	int rdc_update_autoconnect;
	svector *rdc;
};

/* smessage is the structure used for all non-error messages passed to the
	application from the wrapper, via the library: */

class smessage : public sproto
{
	public:

	smessage();
	~smessage();

	int reveal(AbstractMessage *abst); // -1 = bad msg type, -2 = bad xml, 0 = OK
	AbstractMessage *wrap(int fd);
						
	MessageType type; // MessageRcv, MessageResponse or MessageUnavailable
							// Or OOB type MessageReturnCode
	char *source_cpt;
	char *source_inst;
	char *source_ep;
	int source_ep_id;
	char *topic; // NULL if none specified, or for RPCs
	int seq;
	HashCode *hc;
	snode *tree;
	void *state; // For conversations only - not implemented yet
	
	int reason; // MessageUnavailable only
	
	// Out of bound alternative (if set, overrides rest of this class):
	sreturncode *oob_returncode;
	
	private:
	
	char *xml; // Managed internally by smessage - callers just set/get tree
};

/* sinternal is the general structure for all messages passed from the
	library to the wrapper: */

class sinternal : public sproto
{
	public:

	sinternal();
	~sinternal();

	int reveal(AbstractMessage *abst); // returns -1 if wrong msg type, else 0
	AbstractMessage *wrap(int fd);
						
	MessageType type; // MessageEmit, MessageRPC, MessageReply
	// Or OOB types MessageMap, MessageUnmap, MessageIsmap,
	//              MessageSubscribe
	char *topic; // MessageEmit only
	int seq; // MessageReply only
	HashCode *hc;
	char *xml;
	
	// Out of bound messages (if set, overrides rest of this class):
	scontrol *oob_ctrl;
};

/* read_bootupdate() returns
	MessageAddEndpoint = filled add, MessageStop = filled stop,
	MessageGetStatus = filled hook,
	-1 = disconnect, -2 = bad protocol (the latter for sock version only): */

int read_bootupdate(int sock, saddendpoint *add, sstopwrapper *stop,
		shook *hook, srdc *rdc);
int read_bootupdate(AbstractMessage *abst, saddendpoint *add,
		sstopwrapper *stop, shook *hook, srdc *rdc);
	
// Return 0 = filled add, 1 = filled start, -1 = disconnect, -2 = bad protocol:
int read_startup(int sock, saddendpoint *add, sstartwrapper *start);
// Return 0 = filled add, 1 = filled start, -1 = neither type matches:
int read_startup(AbstractMessage *abst, saddendpoint *add,
		sstartwrapper *start);

class ipaddress
{
	/* A set of routines to manipulate addresses in these forms:
	   dns.name
		dns.name:portno
		n.n.n.n
		n.n.n.n:portno
	*/
	
	public:
	
	static char *check_add_port(const char *addr, int port);
	static int has_port(const char *addr);
	// static int valid(const char *addr);
	// static int get_port(const char *addr);
	// static char *get_machine(const char *addr);
};

const char *inet6_ntoa(const struct in6_addr a);

// For peernet:

int decode_count(unsigned char **pos);
char *decode_string(unsigned char **pos);
int decode_byte(unsigned char **pos);
HashCode *decode_hashcode(unsigned char **pos);
StringBuf *begin_msg(MessageType type);
