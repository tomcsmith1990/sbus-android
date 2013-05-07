import java.util.*;
import java.io.*;
import java.nio.channels.*;

enum CptUniqueness
{
	Multiple, Single, Replace;
};

enum EndpointType
{
	Server, Client, Source, Sink;
};

class Component
{
	Component(String cpt_name, String instance_name, CptUniqueness unique)
	{
		construct(cpt_name, instance_name, unique);
	}
	Component(String cpt_name, String instance_name)
	{
		construct(cpt_name, instance_name, CptUniqueness.Multiple);
	}
	Component(String cpt_name)
	{
		construct(cpt_name, null, CptUniqueness.Multiple);
	}
			
	private void construct(String cpt_name, String instance_name,
			CptUniqueness unique)
	{
		name = cpt_name;
		if(instance_name == null)
			instance = cpt_name;
		else
			instance = instance_name;
		Log.init_logfile(cpt_name, instance_name, false);
		epv = new Vector<Endpoint>();
		biv = new Vector<Builtin>();
		rdc = new Vector<String>();
		listen_port = -1; // Not running yet
		uniq = unique;
		canonical_address = null;

		String rdc_path;
		rdc_path = System.getenv("SBUS_RDC_PATH");
		if(rdc_path != null)
		{
			String[] ss = rdc_path.split(",");

			for(int i = 0; i < ss.length; i++)
				rdc.add(ss[i]);
		}

		start_wrapper();
	}
	
	// In Java binding, must call stop() explicitly instead of a destructor
		
	Endpoint add_endpoint(String name, EndpointType type, String msg_hash)
	{
		return add_endpoint(name, type, msg_hash, null);
	}
	
	Endpoint add_endpoint(String name, EndpointType type, String msg_hash,
			String reply_hash)
	{
		HashCode msg_hc, reply_hc;

		msg_hc = new HashCode();
		msg_hc.fromstring(msg_hash);

		reply_hc = new HashCode();
		if(reply_hash == null)
			reply_hc.frommeta(MetaType.SCHEMA_NA);
		else
			reply_hc.fromstring(reply_hash);

		return do_add_endpoint(name, type, msg_hc, reply_hc);
	}
	
	Endpoint add_endpoint(String name, EndpointType type, HashCode msg_hc)
	{
		return add_endpoint(name, type, msg_hc, null);
	}
	
	Endpoint add_endpoint(String name, EndpointType type, HashCode msg_hc,
			HashCode reply_hc)
	{
		HashCode msg_hc_copy, reply_hc_copy;

		msg_hc_copy = new HashCode(msg_hc);
		if(reply_hc == null)
		{
			reply_hc_copy = new HashCode();
			reply_hc_copy.frommeta(MetaType.SCHEMA_NA);
		}
		else
			reply_hc_copy = new HashCode(reply_hc);

		return do_add_endpoint(name, type, msg_hc_copy, reply_hc_copy);
	}
		
	void add_rdc(String address)
	{
		rdc.add(address);
	}

	void start(String metadata_filename)
	{
		start(metadata_filename, -1);
	}
	
	void start(String metadata_filename, int port)
	{
		ProtoStartWrapper start;
		/*
		uid_t id;
		struct passwd *pw;
		*/

		start = new ProtoStartWrapper();
		start.cpt_name = name;
		start.instance_name = instance;

		/*
		id = getuid();
		pw = getpwuid(id);
		if(pw == null)
			Log.error("Can't lookup user name to set creator field");
		start.creator = sdup(pw.pw_gecos);
		// Remove trailing commas:
		int pos = 0;
		while(start.creator[pos] != '\0')
		{
			if(start.creator[pos] == ',')
			{
				start.creator[pos] = '\0';
				break;
			}
			pos++;
		}
		*/
		start.creator = "Java";

		start.metadata_address = metadata_filename;
		if(port < 0)
			start.listen_port = 0;
		else
			start.listen_port = port;
		start.unique = uniq;
		start.log_level = Log.log_level;
		start.echo_level = Log.echo_level;
		start.rdc = new Vector<String>();
		for(int i = 0; i < rdc.size(); i++)
			start.rdc.add(rdc.get(i));
		if(start.write(bootstrap_sc) < 0)
			Log.error("Bootstrap connection to wrapper disconnected in start");

		running();
	}
	
	void stop()
	{
		ProtoStopWrapper packet;

		packet = new ProtoStopWrapper();
		packet.reason = 0;
		packet.write(bootstrap_sc);

		// XXXXXXX
		// Wait for wrapper to exit (as indicated by closing pipe):
		/*
		char buf;
		int bytes;

		bytes = read(bootstrap_fd, &buf, 1);
		if(bytes != 0)
		{
			Log.warning("Warning: wrapper returned data after being " +
					"requested to stop");
		}
		*/
	}

	Endpoint clone(Endpoint ep) // Convenience method, wraps add_endpoint()
	{
		return add_endpoint(ep.name, ep.type, ep.msg_hc, ep.reply_hc);
	}

	int count_endpoints()
	{
		return epv.size();
	}

	Endpoint get_endpoint(int index)
	{
		if(index < 0 || index >= epv.size())
			Log.error("No such endpoint");
		return epv.get(index);
	}

	Endpoint get_endpoint(String name)
	{
		Endpoint ep;

		for(int i = 0; i < epv.size(); i++)
		{
			ep = epv.get(i);
			if(name.equals(ep.name))
				return ep;
		}
		return null;
	}

	Endpoint sc_to_endpoint(SocketChannel sc)
	{
		Endpoint ep;

		for(int i = 0; i < epv.size(); i++)
		{
			ep = epv.get(i);
			if(ep.sc == sc)
				return ep;
		}
		return null;
	}
	
	static void set_log_level(int log, int echo)
	{
		Log.log_level = log;
		Log.echo_level = echo;
	}
			
	// The following are valid after start() has returned:
	
	int count_builtins()
	{
		return biv.size();
	}

	Builtin get_builtin(int index)
	{
		return biv.get(index);
	}
	
	int get_listen_port()
	{
		return listen_port;
	}

	String get_address()
	{
		return canonical_address;
	}

	Node get_status()
	{
		ProtoHook hook;
		ProtoGeneric status;

		hook = new ProtoHook(MessageType.GetStatus);
		if(hook.write(bootstrap_sc) < 0)
		{
			Log.error("Bootstrap connection to wrapper disconnected in " +
					"get_status()");
		}

		status = new ProtoGeneric();
		if(status.read(bootstrap_sc) < 0)
			Log.error("Error reading status message from wrapper");
		if(status.type != MessageType.Status)
			Log.error("Expected status return from wrapper");
		return status.tree;
	}
	
	String get_schema(HashCode hc)
	{
		ProtoHook hook;
		ProtoGeneric sg;
		String hash;

		hash = hc.toString();
		hook = new ProtoHook(MessageType.GetSchema);
		hook.hc = new HashCode();
		hook.hc.fromstring("D3C74D1897A3"); /* @txt hash */
		hook.tree = Node.pack(hash, "hash");

		if(hook.write(bootstrap_sc) < 0)
			Log.error("Bootstrap connection to wrapper disconnected in " +
					"get_schema()");

		sg = new ProtoGeneric();
		if(sg.read(bootstrap_sc) < 0)
			Log.error("Error reading schema message from wrapper");
		if(sg.type != MessageType.Schema)
			Log.error("Expected schema return from wrapper, got type " + sg.type);
		return sg.tree.extract_txt(); /* @txt schema */
	}

	HashCode declare_schema(String schema)
	{
		return do_declare_schema(schema, false);
	}

	HashCode load_schema(String file)
	{
		return do_declare_schema(file, true);
	}

	private String name, instance;
	private CptUniqueness uniq;
	
	// Listens for new connections from the wrapper:
	private ServerSocketChannel callback_ssc;
	// First connection, creates endpoint pipes:
	private SocketChannel bootstrap_sc;

	private int listen_port;
	private String canonical_address;
	
	private void start_wrapper()
	{
		int pid = 666;
		int callback_port;
		LowLevel.ServerSocketPort ssp;
		
		ssp = LowLevel.passivesock(-1, true);
		callback_ssc = ssp.ssc;
		callback_port = ssp.port;

		/* Set up some command-line arguments and exec the child process
			(which becomes the wrapper): */
		
		String[] newargs = new String[2];
		
		newargs[0] = "sbuswrapper";
		newargs[1] = ":" + callback_port;
		Log.debug("Exec invoking wrapper as follows: \"" +
				newargs[0] + " " + newargs[1] + "\"");
		
		Process p;
		Runtime runtime = Runtime.getRuntime();
		try
		{
			p = runtime.exec(newargs);
		}
		catch(IOException e)
		{
			Log.error("Could not run " + newargs[0] +
					": check it is in your path!");
		}
		
		// Parent process continues (return to application):
		LowLevel.SocketRemoteAddr sra;
		sra = LowLevel.acceptsock(callback_ssc);
		bootstrap_sc = sra.sc;
		if(bootstrap_sc == null)
			Log.error("Couldn't accept connection back from wrapper");
		Log.log("Wrapper (PID " + pid + ") running and connected to library");
		
		/*
			TODO - Return PID over pipe.
			PID can be found as described here:
			http://blog.igorminar.com/2007/03/
				how-java-application-can-discover-its.html
			(only works on Unix, and requires bash).
			XXXX
		*/
	}

	private void running()
	{
		// Read ProtoRunning:
		ProtoRunning srun;
		Node builtins, subnode;
		Builtin bi;
		String type;
		int index;

		srun = new ProtoRunning();
		if(srun.read(bootstrap_sc) < 0)
			Log.error("Error reading ProtoRunning message from wrapper");
		listen_port = srun.listen_port;
		builtins = srun.builtins;
		canonical_address = srun.address;

		for(int i = 0; i < builtins.count(); i++)
		{
			bi = new Builtin();
			subnode = builtins.extract_item(i);
			bi.name = subnode.extract_txt("name");
			type = subnode.extract_txt("type");
			try
			{
				bi.type = EndpointType.valueOf(Utils.mixed_case(type));
			}
			catch(Exception e)
			{
				Log.error("Builtin endpoint has illegal type " + type);
			}
			bi.msg_hc = new HashCode();
			bi.reply_hc = new HashCode();
			bi.msg_hc.fromstring(subnode.extract_txt("msg-hash"));
			bi.reply_hc.fromstring(subnode.extract_txt("reply-hash"));
			biv.add(bi);
		}
	}

	private HashCode do_declare_schema(String schema, boolean file_lookup)
	{
		ProtoHook hook;
		ProtoGeneric sg;
		HashCode hc;

		hook = new ProtoHook(MessageType.Declare);
		hook.hc = new HashCode();
		hook.hc.fromstring("3D79D04FEBCC");
		// @declare { txt schema flg file_lookup }
		hook.tree = Node.pack(Node.pack(schema, "schema"),
				Node.pack(file_lookup, "file_lookup"), "declare");

		if(hook.write(bootstrap_sc) < 0)
		{
			Log.error("Bootstrap connection to wrapper disconnected in " +
					"load_schema()");
		}

		sg = new ProtoGeneric();
		if(sg.read(bootstrap_sc) < 0)
			Log.error("Error reading schema hash message from wrapper");
		if(sg.type != MessageType.Hash)
		{
			Log.error("Expected schema hash return from wrapper, got type " +
					sg.type);
		}
		hc = new HashCode();
		hc.fromstring(sg.tree.extract_txt()); /* @txt hash */

		return hc;
	}
	
	private Endpoint do_add_endpoint(String name, EndpointType type,
			HashCode msg_hc, HashCode reply_hc)
	{
		// Note: this function consumes its msg_hc and reply_hc arguments
		Endpoint ep;
		ProtoAddEndpoint add;

		/* N.B. if listen_port is not -1, the component has started;
			add_endpoint() is then still allowed, but the endpoint is
			not matched with anything from the component metadata. */

		if(!reply_hc.isapplicable() && (type.equals(EndpointType.Client) ||
				type.equals(EndpointType.Server)))
			Log.error("Endpoint requires a response, but reply hash code not set");
		if(reply_hc.isapplicable() && (type.equals(EndpointType.Source) ||
				type.equals(EndpointType.Sink)))
			Log.error("Endpoint has no response, but a reply hash code was given");

		ep = new Endpoint(name, type, msg_hc, reply_hc);

		// Send MessageType.AddEndpoint:
		add = new ProtoAddEndpoint();
		add.endpoint = name;
		add.type = type;
		add.msg_hc = msg_hc;
		add.reply_hc = reply_hc;
		if(add.write(bootstrap_sc) < 0)
		{
			Log.error("Bootstrap connection to wrapper disconnected in "
					+ "add_endpoint");
		}

		LowLevel.SocketRemoteAddr sra;
		sra = LowLevel.acceptsock(callback_ssc);
		ep.sc = sra.sc;
		if(ep.sc == null)
			Log.error("Couldn't accept additional connection back from wrapper");

		epv.add(ep);
		return ep;
	}
	
	private Vector<Builtin> biv;
	private Vector<Endpoint> epv;
	private Vector<String> rdc;
};

class Builtin
{
	Builtin()
	{
		name = null;
		msg_hc = reply_hc = null;
	}

	String name;
	EndpointType type;
	HashCode msg_hc, reply_hc;
};

class Endpoint
{
	Endpoint(String name, EndpointType type,
		HashCode msg_hc, HashCode reply_hc)
	{
		this.name = name;
		this.type = type;
		this.msg_hc = new HashCode(msg_hc);
		this.reply_hc = new HashCode(reply_hc);
		postponed_queue = new LinkedList<ProtoMessage>();
	}

	/* map() returns a string containing the address of the
		other component if successful, otherwise null.
		The caller must delete this string in the successful case.
		If endpoint is null, assume same name as this end: */
	
	String map(String address, String endpoint)
	{
		return map(address, endpoint, 0, null);
	}
	
	String map(String address, String endpoint, int sflags, String pub_key)
	{
		ProtoControl ctrl;
		ProtoMessage msg;
		ProtoReturnCode rc;

		ctrl = new ProtoControl();
		ctrl.type = MessageType.Map;
		if(endpoint == null)
			ctrl.target_endpoint = name; // Assume same name as other end
		else
			ctrl.target_endpoint = endpoint;
		ctrl.address = address;
		if(ctrl.write(sc) < 0)
			Log.error("Control connection to wrapper disconnected in map");

		msg = read_returncode();
		rc = msg.oob_returncode;
		if(rc.retcode != 1)
			return null; // Map failed

		return rc.address; // OK
	}
	
	void unmap()
	{
		ProtoControl ctrl;
		ProtoMessage msg;

		ctrl = new ProtoControl();
		ctrl.type = MessageType.Unmap;
		ctrl.address = null;
		if(ctrl.write(sc) < 0)
			Log.error("Control connection to wrapper disconnected in unmap");

		msg = read_returncode();
		// No data to make use of; just had to wait for it to arrive
	}

	int ismapped()
	{
		ProtoControl ctrl;
		ProtoMessage msg;
		ProtoReturnCode rc;
		int n;

		ctrl = new ProtoControl();
		ctrl.type = MessageType.Ismap;
		ctrl.address = null;
		if(ctrl.write(sc) < 0)
			Log.error("Control connection to wrapper disconnected in ismap");

		msg = read_returncode();
		rc = msg.oob_returncode;	
		n = rc.retcode;

		return n;
	}

	String map(MapConstraints constraints, int sflags)
	{
		Log.error("Mapping with constraints structure not implemented yet");
		return null; // Failed
	}
	
	void subscribe(String topic, String subs, String peer)
	{
		ProtoControl ctrl;

		ctrl = new ProtoControl();
		ctrl.type = MessageType.Subscribe;
		ctrl.subs = subs;
		ctrl.topic = topic;
		if(ctrl.write(sc) < 0)
			Log.error("Control connection to wrapper disconnected in subscribe");
	}
	
	/* "peer" may be a component or instance name, or "*" for all.
		If it is null, no currently mapped peer is resubscribed but the
		default subscription to use for future maps is set. */
	
	/* client calls rpc(), server calls rcv() & reply(),
		source calls emit(), sink calls rcv(): */
	
	ProtoMessage rcv()
	{
		ProtoMessage inc;
		int ret;

		if(postponed_queue.size() > 0)
		{
			inc = postponed_queue.remove();
			if(inc.type != MessageType.Rcv)
			{
				Log.error("Incorrect pre-fetched message type in receive from " +
						"wrapper");
			}
			return inc;
		}	
		inc = new ProtoMessage();
		ret = inc.read(sc);
		if(ret == -1)
			Log.wrapper_failed();
		else if(ret < 0)
			Log.error("Error reading incoming data from wrapper");

		if(inc.type == MessageType.ReturnCode)
			Log.error("Return code received, but not within a map/unmap/ismap");
		if(inc.type != MessageType.Rcv)
			Log.error("Incorrect message type in receive from wrapper");

		return inc;
	}

	void reply(ProtoMessage query, Node result)
	{
		reply(query, result, null);
	}
	
	/*
	void reply(ProtoMessage query, Node result, int exception, HashCode hc)
	*/
	
	void reply(ProtoMessage query, Node result, HashCode hc)
	{
		ProtoInternal sreq;

		sreq = new ProtoInternal();
		sreq.type = MessageType.Reply;
		sreq.topic = null;
		sreq.seq = query.seq;
		if(hc == null)
			sreq.hc = new HashCode(reply_hc);
		else
			sreq.hc = new HashCode(hc);
		sreq.xml = XML.toxml(result, false);
		if(sreq.write(sc) < 0)
			Log.wrapper_failed();
	}
	
	void emit(Node node)
	{
		emit(node, null, null);
	}
	
	void emit(Node node, String topic)
	{
		emit(node, topic, null);
	}
	
	void emit(Node node, HashCode hc)
	{
		emit(node, null, hc);
	}
	
	void emit(Node node, String topic, HashCode hc)
	{
		ProtoInternal sreq;

		sreq = new ProtoInternal();
		sreq.type = MessageType.Emit;
		sreq.topic = topic;
		sreq.seq = 0;
		if(hc == null)
			sreq.hc = new HashCode(msg_hc);
		else
			sreq.hc = new HashCode(hc);
		if(node == null)
			sreq.xml = "0";
		else
			sreq.xml = XML.toxml(node, false);
		if(sreq.write(sc) < 0)
			Log.wrapper_failed();
	}

	ProtoMessage rpc(Node query)
	{
		return rpc(query, null);
	}
	
	ProtoMessage rpc(Node query, HashCode hc)
	{
		ProtoInternal sreq;
		ProtoMessage msg;
		int ret;

		sreq = new ProtoInternal();
		sreq.type = MessageType.RPC;
		sreq.topic = null;
		sreq.seq = 0;
		if(hc == null)
			sreq.hc = new HashCode(msg_hc);
		else
			sreq.hc = new HashCode(hc);
		if(query == null)
			sreq.xml = "0";
		else
			sreq.xml = XML.toxml(query, false);
		if(sreq.write(sc) < 0)
			Log.wrapper_failed();

		// Receive reply:

		msg = new ProtoMessage();
		ret = msg.read(sc);
		if(ret == -1)
			Log.wrapper_failed();
		else if(ret < 0)
			Log.error("Error reading response from wrapper");

		if(msg.type == MessageType.Unavailable)
			return null;
		else if(msg.type == MessageType.ReturnCode)
			Log.error("Return code received, but not within a map/unmap/ismap");
		else if(msg.type != MessageType.Response)
			Log.error("Incorrect message type in response from wrapper");

		return msg;
	}
	
	int message_waiting() // Returns the number of pre-fetched messages
	{
		return postponed_queue.size();
	}
	
	String name;
	EndpointType type;
	
	SocketChannel sc; // Used by the library to talk to the wrapper

	HashCode msg_hc, reply_hc; // Specified when defined; type checks
	
	private ProtoMessage read_returncode()
	{
		// Read map/unmap/ismap return value (may have to buffer incoming data):
		ProtoMessage msg;

		while(true)
		{
			msg = new ProtoMessage();
			if(msg.read(sc) < 0)
				Log.error("Error reading map success value from wrapper");
			if(msg.type == MessageType.ReturnCode)
				break;
			postponed_queue.add(msg);
		}
		return msg;
	}
	
	private LinkedList<ProtoMessage> postponed_queue;
};

enum TiePolicy { Random, Load };
enum MatchMode { Partial, Exact, Contained };

class MapConstraints
{
	MapConstraints()
	{
		init();
	}

	MapConstraints(String string)
	{
		char code;
		StringScanner sc = new StringScanner(string);

		init();
		while(!sc.eof())
		{
			if(sc.getchar() != '+') { failed_parse = 1; return; }
			sc.advance();
			if(sc.eof()) { failed_parse = 1; return; }
			code = sc.getchar();
			sc.advance();
			while(sc.getchar() == ' ' || sc.getchar() == '\t')
				sc.advance();
			switch(code)
			{
				case 'N':
					cpt_name = read_word(sc);
					break;
				case 'I':
					instance_name = read_word(sc);
					break;
				case 'U':
					creator = read_word(sc);
					break;
				case 'X':
					pub_key = read_word(sc);
					break;
				case 'K':
					keywords.add(read_word(sc));
					break;
				case 'P':
					peers.add(read_word(sc));
					break;
				case 'A':
					ancestors.add(read_word(sc));
					break;
				default:
					failed_parse = 1; return;
			}
			while(sc.getchar() == ' ' || sc.getchar() == '\t')
				sc.advance();
		}
	}
	
	private String read_word(StringScanner sc)
	{
		int start, end;
		
		start = sc.getpos();
		while(!sc.eof() && sc.getchar() != ' ' && sc.getchar() != '\t' &&
				sc.getchar() != '+')
		{
			sc.advance();
		}
		end = sc.getpos();
		
		return sc.substring(start, end);
	}
			
	/* is_constraint returns 1 if the string looks like a constraint string,
		0 if it looks like host and/or port, and -1 if it resembles
		neither of these (and hence is definitely an invalid address).
		Note: the string may still fail to pass more detailed parsing checks
		in cases 0 or 1: */
			
	static int is_constraint(String string)
	{
		int colon = 0, plus = 0;
		int pos = 0, len = string.length();
		
		while(pos < len)
		{
			if(string.charAt(pos) == ':')
				colon++;
			else if(string.charAt(pos) == '+')
				plus++;
			pos++;
		}
		if(colon == 1 && plus == 0)
			return 0;
		if(colon == 0 && plus > 0)
			return 1;
		return -1;
	}
	
	Node pack()
	{
		Node node, subnode;

		node = Node.mklist("map-constraints");
		// The next four lines also work correctly if any string is null
		node.append(Node.pack(cpt_name, "cpt-name"));
		node.append(Node.pack(instance_name, "instance-name"));
		node.append(Node.pack(creator, "creator"));
		node.append(Node.pack(pub_key, "pub-key"));
		subnode = Node.mklist("keywords");
		for(int i = 0; i < keywords.size(); i++)
			subnode.append(Node.pack(keywords.get(i), "keyword"));
		node.append(subnode);
		subnode = Node.mklist("parents");
		for(int i = 0; i < peers.size(); i++)
			subnode.append(Node.pack(peers.get(i), "cpt"));
		node.append(subnode);
		subnode = Node.mklist("ancestors");
		for(int i = 0; i < ancestors.size(); i++)
			subnode.append(Node.pack(ancestors.get(i), "cpt"));
		node.append(subnode);
		node.append(Node.pack(match_endpoint_names, "match-endpoint-names"));
		return node;
	}

	void set_name(String s)
	{ cpt_name = s; }

	void set_instance(String s)
	{ instance_name = s; }

	void set_creator(String s)
	{ creator = s; }

	void set_key(String s)
	{ pub_key = s; }

	void add_keyword(String s)
	{ keywords.add(s); }

	void add_peer(String s)
	{ peers.add(s); }

	void add_ancestor(String s)
	{ ancestors.add(s); }

	int match_endpoint_names;

	// Note: the next two fields are currently unsupported
	MatchMode match_mode;
	TiePolicy tie_policy; // Used if several components match
	
	int failed_parse;
	
	private void init()
	{
		cpt_name = instance_name = creator = pub_key = null;
		keywords = new Vector<String>();
		peers = new Vector<String>();
		ancestors = new Vector<String>();

		match_endpoint_names = 0;
		match_mode = MatchMode.Exact;
		tie_policy = TiePolicy.Random;
		failed_parse = 0;
	}

	private Vector<String> keywords;
	private Vector<String> peers, ancestors;
	
	// Any of these may be null to indicate irrelevant:
	private String cpt_name;
	private String instance_name;
	private String creator;
	private String pub_key;	
};

/*
// Values for sflags:
const int UnavailableSilent = 1 << 0;
const int DisconnectSilent = 1 << 1;
const int AttemptFailover = 1 << 2;
const int AttemptReconnect = 1 << 3;
const int AllowMigration = 1 << 4;
const int AwaitTransfer = 1 << 5;
*/
