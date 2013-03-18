// peernet.h - DMI - 9-5-2009

class sgoodbye
{
	public:
			
	sgoodbye();
	~sgoodbye();
	
	AbstractMessage *wrap(int fd);

	char *src_cpt, *src_ep;
	char *tgt_cpt, *tgt_ep;	
};

class sresub
{
	public:
			
	sresub();
	~sresub();
	
	AbstractMessage *wrap(int fd);

	char *src_cpt, *src_ep;
	char *tgt_cpt, *tgt_ep;
	
	char *subscription, *topic;
};

class sdivert
{
	public:
			
	sdivert();
	~sdivert();
	
	AbstractMessage *wrap(int fd);
	
	char *src_cpt, *src_ep;
	char *tgt_cpt, *tgt_ep;	
	char *new_cpt, *new_ep;
};

class sflow
{
	public:
			
	sflow();
	~sflow();
	
	int reveal(AbstractMessage *abst); // returns -1 if wrong msg type, else 0
	AbstractMessage *wrap(int fd);
	
	char *src_cpt, *src_ep;
	char *tgt_cpt, *tgt_ep;
	int window;
};

class svisitor
{
	public:
	
	svisitor();
	~svisitor();
	
	AbstractMessage *wrap(int fd);
	
	char *src_cpt, *tgt_cpt, *src_instance;
	char *required_endpoint;
	int dispose_id;
	EndpointType ep_type; // EndpointSink or EndpointServer
	HashCode *msg_hc;
	HashCode *reply_hc;
	int length;
	unsigned char *data;
};

/* scomm is the general structure for all component to component messages,
	after connection establishment has taken place: */

class scomm
{
	public:
	
	scomm();
	scomm(svisitor *visitor);
	~scomm();
	
	/* Notes: read() or reveal() may also read an OOB message.
		Do not use the write() or wrap() method here to send OOB messages
		though; use the specific type's write() or wrap() method instead. */
	
	int reveal(AbstractMessage *abst); // returns -1 if wrong msg type, else 0
	AbstractMessage *wrap(int fd);
	
	char *source;
	char *src_endpoint;
	char *target;
	char *tgt_endpoint;
	MessageType type; // MessageSink, MessageServer, MessageClient
	// or OOB types: MessageGoodbye, MessageResubscribe, MessageDivert
	char *topic; // NULL if not MessageSink, and might be NULL anyway
	int seq;
	HashCode *hc;
	int length;
	unsigned char *data;

	// Out of bound messages (if set, overrides rest of this class):
	sgoodbye *oob_goodbye;
	sresub *oob_resub;
	sdivert *oob_divert;
		
	// Only for use inside the wrapper, not transmitted across the network:
	int peer_uid;

	//this is a very nasty hack - to deal with temporary connections (see serve_visitor in wrapper)
	int terminate_disposable;
};

class shello
{
	public:

	shello();
	~shello();

	/* Note: reveal() may also read a disposable message (type Visitor).
		Do not use the wrap() method here for disposables however; use
		the svisitor specific wrap() method. */
	
	int reveal(AbstractMessage *abst); // returns -1 if wrong msg type, else 0
	AbstractMessage *wrap(int fd);

	char *source; // cpt name
	char *from_address;
	char *from_instance;
	char *from_endpoint;
	int from_ep_id;
	char *target; // cpt name; may be NULL if not specified
	char *required_endpoint;
	int required_ep_id;
	EndpointType target_type;
	char *subs;
	char *topic;
	HashCode *msg_hc;
	HashCode *reply_hc;
	
	int flexible_matching;

	// If non-NULL, overrides rest of this class:	
	svisitor *oob_visitor;
};

class swelcome
{
	public:
			
	swelcome();
	~swelcome();
	
	int reveal(AbstractMessage *abst); // returns -1 if wrong msg type, else 0
	AbstractMessage *wrap(int fd);

	AcceptanceCode code;
	char *cpt_name;
	char *instance;
	char *endpoint;
	char *address;
	char *subs;
	char *topic;
	int msg_poly, reply_poly;
	
	HashCode *msg_hc;
	HashCode *reply_hc;
};
