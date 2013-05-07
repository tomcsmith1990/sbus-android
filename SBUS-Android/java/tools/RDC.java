import java.io.*;
import java.nio.*;
import java.nio.channels.*;
import java.util.*;
import java.util.concurrent.locks.*;

class Image
{
	String address;
	Node metadata, state;
	Vector<String> msg_hsh, reply_hsh;
	
	boolean local, persistent, lost;
	long tv_ping;
	
	Image()
	{
		address = null;
		metadata = state = null;
		msg_hsh = new Vector<String>();
		reply_hsh = new Vector<String>();
		lost = false;
	}

	void init_hashes(Component com)
	{
		Node sn, subn;
		HashCode hc;
		int endpoints;

		sn = metadata.extract_item("endpoints");
		endpoints = sn.count();
		for(int i = 0; i < endpoints; i++)
		{
			subn = sn.extract_item(i);
			hc = com.declare_schema(subn.extract_txt("message"));
			msg_hsh.add(hc.toString());
			hc = com.declare_schema(subn.extract_txt("response"));
			reply_hsh.add(hc.toString());
		}
		// Sanity check:
		if(msg_hsh.size() != endpoints || reply_hsh.size() != endpoints)
			Log.error("Number of endpoints assertion failed in RDC");
	}
				
	boolean match(Node interFace, Node constraints, Component com)
	{
		boolean match_endpoint_names;
		Node sn, search, subsearch;
		Node ep_reqd, ep_actual, ep_live;
		Builtin bi;
		String value, s;
		boolean match;
		int pos;

		// Check simple constraints:

		match_endpoint_names = constraints.extract_flg("match-endpoint-names");
		if(constraints.exists("cpt-name"))
		{
			value = constraints.extract_txt("cpt-name");
			if(!value.equals(metadata.extract_txt("name")))
				return false;
		}
		if(constraints.exists("instance-name"))
		{
			value = constraints.extract_txt("instance-name");
			if(!value.equals(state.extract_txt("instance")))
				return false;
		}
		if(constraints.exists("creator"))
		{
			value = constraints.extract_txt("creator");
			if(!value.equals(state.extract_txt("creator")))
				return false;
		}
		if(constraints.exists("pub-key"))
		{
			value = constraints.extract_txt("pub-key");
			Log.warning("Public key constraints are unimplemented.");
		}
		sn = constraints.extract_item("keywords");
		search = metadata.extract_item("keywords");
		for(int i = 0; i < sn.count(); i++)
		{
			// For each keyword...
			value = sn.extract_txt("keyword");
			for(pos = 0; pos < search.count(); pos++)
				if(value.equals(search.extract_txt(pos)))
					break;
			if(pos == search.count())
				return false;
		}

		// Check interface type compatibility:

		search = metadata.extract_item("endpoints");
		// Loop through all the endpoints specified by the requested interface:
		for(int i = 0; i < interFace.count(); i++)
		{
			ep_reqd = interFace.extract_item(i);
			match = false;
			// Loop through all this cpt image's endpoints, to see if satisfy:
			for(int j = 0; j < search.count(); j++)
			{
				ep_actual = search.extract_item(j);

				// Check names, if applicable:
				if(match_endpoint_names && ep_reqd.exists("name") &&
					!ep_reqd.extract_txt("name").equals(
						ep_actual.extract_txt("name")))
				{
						continue;
				}
				// Check types:
				if(!ep_actual.extract_value("type").equals(
						ep_reqd.extract_value("type")))
				{
					continue;
				}
				// OK so far. Now for the LITMUS tests:
				if(!hashmatch(ep_reqd.extract_txt("msg-hash"), msg_hsh.get(j)))
					continue;
				if(!hashmatch(ep_reqd.extract_txt("reply-hash"), reply_hsh.get(j)))
					continue;
				match = true;
				break; // This endpoint matches
			}
			if(match) continue;

			// Maybe it's a built-in...?
			for(int j = 0; j < com.count_builtins(); j++)
			{
				bi = com.get_builtin(j);

				// Check names, if applicable:
				if(match_endpoint_names && ep_reqd.exists("name") &&
					!ep_reqd.extract_txt("name").equals(bi.name))
				{
					continue;
				}
				// Check types:
				if(!bi.type.toString().equalsIgnoreCase
						(ep_reqd.extract_value("type")))
				{
					continue;
				}
				// OK so far. Now for the LITMUS tests:
				s = bi.msg_hc.toString();
				if(!hashmatch(ep_reqd.extract_txt("msg-hash"), s))
					continue;
				s = bi.reply_hc.toString();
				if(!hashmatch(ep_reqd.extract_txt("reply-hash"), s))
					continue;
				match = true;
				break; // This endpoint matches
			}
			if(match)
				continue;
			return false; // Couldn't find a match for this interface endpoint
		}

		// Check connected peer component constraints:

		sn = constraints.extract_item("parents");
		search = state.extract_item("endpoints");
		for(int i = 0; i < sn.count(); i++)
		{
			// For each required peer constraint...
			value = sn.extract_txt("cpt");
			match = false;
			for(int j = 0; j < search.count(); j++)
			{
				ep_live = search.extract_item(j);
				subsearch = ep_live.extract_item("peers");
				for(int k = 0; k < subsearch.count(); k++)
				{
					if(value.equals(
							subsearch.extract_item(k).extract_txt("cpt_name")) ||
						value.equals(
							subsearch.extract_item(k).extract_txt("instance")))
					{
						match = true;
						break;
					}
				}
				if(match)
					break;
			}
			if(!match)
				return false;
		}
		sn = constraints.extract_item("ancestors");
		for(int i = 0; i < sn.count(); i++)
		{
			value = sn.extract_txt("cpt");
			Log.warning("Ancestor constraints are unimplemented.");
		}

		return true;
	}
	
	boolean hashmatch(String hsh1, String hsh2)
	{
		String poly = "FFFFFFFFFFFF";

		if(hsh1.equals(poly) || hsh2.equals(poly))
			return true;
		if(hsh1.equals(hsh2))
			return true;
		return false;
	}
};

class RDC
{
	Component com;
	
	Endpoint events_ep, register_ep, lost_ep;
	Endpoint dump_client_ep;
	Endpoint metadata_ep, status_ep;
	Endpoint terminate_ep;

	ArrayList<Endpoint> metadata_epv, status_epv;
	ArrayList<Boolean> pool_busy;
		
	ArrayList<Image> live;
	ArrayList<String> joins;

	/* Protects the "live" data structure (but not the contents of
		individual images, so there is a possible concurrency issue
		with modifications to them): */
	Lock live_mutex;
	// Protects pool_busy, which in turn guards metadata_epv and status_epv:
	Lock pool_mutex;
	Lock events_mutex; // Arbitrates use of events_ep.emit()

	Persistence persist;		
	private String local_address;	
	long tv_start;
	int port = -1;
	
	static final int max_pool_size = 10;
	static final int check_ms = 5000;
	// static final int check_ms = 50;
	
	RDC(String[] args)
	{
		parse_args(args);
		
		persist = null;
		Component.set_log_level(Log.LogErrors | Log.LogWarnings |
				Log.LogMessages, Log.LogErrors | Log.LogWarnings);
		com = new Component("rdc");

		register_ep = com.add_endpoint("register", EndpointType.Sink,
				"B3572388E4A4");
		lost_ep = com.add_endpoint("lost", EndpointType.Sink,
				"B3572388E4A4");
		com.add_endpoint("hup", EndpointType.Sink,
				"000000000000");
		com.add_endpoint("lookup_cpt", EndpointType.Server,
				"AE7945554959", "6AA2406BF9EC");
		com.add_endpoint("list", EndpointType.Server,
				"000000000000", "46920F3551F9");
		com.add_endpoint("cached_metadata", EndpointType.Server,
				"872A0BD357A6", "6306677BFE43");
		com.add_endpoint("cached_status", EndpointType.Server,
				"872A0BD357A6", "253BAC1C33C7");
		com.add_endpoint("dump", EndpointType.Server,
				"000000000000", "534073C1E375");
		events_ep = com.add_endpoint("events", EndpointType.Source,
				"B3572388E4A4");
		metadata_ep = com.add_endpoint("get_metadata", EndpointType.Client,
				"000000000000", "6306677BFE43");
		status_ep = com.add_endpoint("get_status", EndpointType.Client,
				"000000000000", "253BAC1C33C7");
		dump_client_ep = com.add_endpoint("dump_client", EndpointType.Client,
				"000000000000", "534073C1E375");
		terminate_ep = com.add_endpoint("terminate", EndpointType.Source,
				"000000000000");
		com.add_endpoint("remote_start", EndpointType.Sink,
				"8F720B145518");

		live = new ArrayList<Image>();

		metadata_epv = new ArrayList<Endpoint>();
		status_epv = new ArrayList<Endpoint>();
		pool_busy = new ArrayList<Boolean>();

		live_mutex = new ReentrantLock();
		pool_mutex = new ReentrantLock();
		events_mutex = new ReentrantLock();

		tv_start = System.currentTimeMillis();
	}

	void usage()
	{
		System.out.println("Usage:   java RDC [ <option> ... ]");
		System.out.println("Options: -p <port> = port number");
		System.out.println("         -a        = any port");
		System.out.println("         -j <addr> = join other RDC");
		System.out.println("         -f <file> = try to join multiple RDCs");
		System.exit(0);
	}
	
	void parse_args(String[] args)
	{
		joins = new ArrayList<String>();

		for(int i = 0; i < args.length; i++)
		{
			if(args[i].equals("-p"))
			{
				i++;
				if(i >= args.length)
					usage();
				if(port != -1)
					usage();
				port = Integer.parseInt(args[i]);
				if(port < 1 || port > 65535)
					usage();
			}
			else if(args[i].equals("-a"))
			{
				if(port != -1)
					usage();
				port = 0;
			}
			else if(args[i].equals("-j"))
			{
				i++;
				if(i >= args.length)
					usage();
				joins.add(args[i]);
			}
			else if(args[i].equals("-f"))
			{
				ArrayList<String> lf;

				i++;
				if(i >= args.length)
					usage();
				lf = Utils.readlinefile(args[i], true);
				for(int j = 0; j < lf.size(); j++)
					joins.add(lf.get(j));
			}
			else
				usage();
		}
		if(port == -1)
			port = LowLevel.DEFAULT_RDC_PORT;
	}
	
	public static void main(String[] args)
	{
		RDC obj;

		obj = new RDC(args);
		obj.mainloop();
	}
	
	void mainloop()
	{
		String cpt_filename = "rdc.cpt";
		SocketChannel sc;
		Selector selector = null;
		Iterator<SelectionKey> it;
		SelectionKey key;
		Endpoint ep;
		long tv_lastcheck, tv_now, ms;

		Component.set_log_level(Log.LogAll, Log.LogAll);
		
		if(port == 0)
			com.start(cpt_filename); // Use any port
		else	
			com.start(cpt_filename, port);

		read_local_address();
		persist = new Persistence();
		join();

		try
		{
			selector = Selector.open();
			for(int i = 0; i < com.count_endpoints(); i++)
				com.get_endpoint(i).sc.register(selector, SelectionKey.OP_READ);
		}
		catch(IOException e)
		{
			Log.error("Error setting up a selector.");
		}

		persist.start_all(); // Start persistent components

		// Select loop:
		tv_lastcheck = System.currentTimeMillis();
		while(true)
		{
			tv_now = System.currentTimeMillis();
			ms = tv_now - tv_lastcheck;
			ms = check_ms - ms;
			if(ms <= 0)
			{
				// Timeout: do checkalive
				schedule_check();
				tv_lastcheck = System.currentTimeMillis();
				continue;
			}
			sc = null;
			try
			{ selector.select(ms); }
			catch(IOException e)
			{ Log.error("select() failed."); }
			it = selector.selectedKeys().iterator();
			while(it.hasNext())
			{
				key = it.next();
				sc = (SocketChannel)key.channel();
				ep = com.sc_to_endpoint(sc);
				if(ep == null)
					continue;
				if(ep.name.equals("register")) registercpt();
				else if(ep.name.equals("lost")) lost();
				else if(ep.name.equals("hup"))
					persist.hup(ep, live, live_mutex, terminate_ep);
				else if(ep.name.equals("lookup_cpt")) lookup(ep);
				else if(ep.name.equals("list")) list(ep);
				else if(ep.name.equals("dump")) dump(ep);
				else if(ep.name.equals("remote_start")) remotestart(ep);
            it.remove();
			}
		}
}
	
	boolean is_whitespace(String s)
	{
		int len = s.length();
		char c;

		for(int i = 0; i < len; i++)
		{
			c = s.charAt(i);
			if(c != ' ' && c != '\t' && c != '\n')
				return false;
		}
		return true;
	}

	void join()
	{
		String addr, fulladdr;
		ProtoMessage msg;
		Node sn, subn;	
		int num_cpts;
		String cptaddr;
		Image img;
		boolean known;
		String our_address;
		String remote_address;

		our_address = com.get_address();
		System.out.printf("Our address is %s\n", our_address);
		for(int i = 0; i < joins.size(); i++)
		{
			addr = joins.get(i);
			if(is_whitespace(addr))
				continue;
			
			// Add default port, if not specified by addr:
			fulladdr = IPAddress.check_add_port(addr, LowLevel.DEFAULT_RDC_PORT);

			remote_address = dump_client_ep.map(fulladdr, "dump");
			if(remote_address == null)
			{
				System.out.printf("RDC %s not running; skipping it\n", addr);
				continue;
			}

			// Check we haven't bound to ourselves:
			if(our_address.equals(remote_address))
			{
				System.out.printf("Attempt to connect to %s bound to self; " +
						"skipping it\n", addr);
				dump_client_ep.unmap();
				continue;
			}
			else
				System.out.printf("Joined to RDC at %s\n", remote_address);

			msg = dump_client_ep.rpc(null);
			if(msg == null)
			{
				System.out.printf("Warning: could not dump memory from " +
						"joined RDC %s\n", addr);
				continue;
			}
			sn = msg.tree;
			num_cpts = sn.count();
			for(int j = 0; j < num_cpts; j++)
			{
				// Add component to memory, if not already known:
				subn = sn.extract_item(j);
				cptaddr = subn.extract_txt("address");

				// Check not already known:
				known = false;
				for(int k = 0; k < live.size(); k++)
				{
					img = live.get(k);
					if(img.address.equals(cptaddr))
					{
						known = true;
						break;
					}
				}
				if(known)
					continue;

				img = new Image();
				img.address = cptaddr;
				img.metadata = new Node(subn.extract_item("metadata"));
				img.state = new Node(subn.extract_item("state"));
				img.tv_ping = System.currentTimeMillis();
				img.local = is_local(img.address);
				if(img.local)
				{
					img.persistent =
						persist.is_persistent(img.state.extract_txt("cmdline"));
				}
				else
					img.persistent = false;
				img.init_hashes(com);
				live.add(img);
				System.out.printf("Registered component %s at %s\n",
						img.metadata.extract_txt("name"), img.address);
			}
			dump_client_ep.unmap();

			// Map register to events, and vice versa:
			register_ep.map(fulladdr, "events");
			events_ep.map(fulladdr, "register");
		}
	}
	
	void lost()
	{
		ProtoMessage msg;
		String addr;
		Image img = null;

		msg = lost_ep.rcv();
		addr = msg.tree.extract_txt("address");

		boolean found = false;
		live_mutex.lock();
		try
		{
			for(int i = 0; i < live.size(); i++)
			{
				img = live.get(i);
				if(img.address.equals(addr))
				{
					if(img.lost)
						return;
					found = true;
					break;
				}
			}
			if(!found)
				return;
			System.out.printf("Received report that we have lost contact " +
					"with %s;\n" +
					"   scheduling it for immediate liveness check\n", addr);
			img.lost = true;
			img.tv_ping = tv_start; // Indicate need to check urgently
		}
		finally
		{
			live_mutex.unlock();
		}
		schedule_check(); // Extra non-periodic check
	}
	
	void schedule_check()
	{
		Thread thread;
		int pool_id;

		pool_id = first_free_pool();
		if(pool_id == -1)
			return; // Omit checkalive

		CheckAlive ca = new CheckAlive(this, pool_id);
		thread = new Thread(ca);
		thread.start();
	}

	void registercpt()
	{
		ProtoMessage msg;
		String addr;
		Image img;
		boolean arrived;

		msg = register_ep.rcv();
		addr = msg.tree.extract_txt("address");
		arrived = msg.tree.extract_flg("arrived");

		if(!msg.source_ep.equals("events"))
		{
			// OK, event isn't from another RDC, so we need to broadcast it:
			events_mutex.lock();
			events_ep.emit(msg.tree);
			events_mutex.unlock();
		}

		if(!arrived)
		{
			deregister(addr);
			return;
		}

		// Check not already registered:
		live_mutex.lock();
		for(int i = 0; i < live.size(); i++)
		{
			img = live.get(i);
			if(img.address.equals(addr))
			{
				live_mutex.unlock();
				System.out.printf("Warning: asked to register " +
						"already-registered component %s.\n", img.address);
				return;
			}
		}	
		live_mutex.unlock();

		img = new Image();
		img.address = addr;
		img.local = is_local(img.address);
		img.persistent = false; // Until we know better, from status structure

		// Call component back to get status and metadata:
		Thread thread;
		int pool_id;

		pool_id = first_free_pool();
		if(pool_id == -1)
		{
			// Drop registration on the floor
			return;
		}
		RegisterBack rb = new RegisterBack(this, pool_id, img);
		thread = new Thread(rb);
		thread.start();
	}
	
	void deregister(String addr)
	{
		Image img = null;
		int pos;

		live_mutex.lock();
		try
		{
			for(pos = 0; pos < live.size(); pos++)
			{
				img = live.get(pos);
				if(img.address.equals(addr))
					break;
			}
			if(pos == live.size())
				return;
			live.remove(pos);
		}
		finally
		{ live_mutex.unlock(); }

		System.out.printf("Deregistered component %s at %s\n",
				img.metadata.extract_txt("name"), img.address);
		if(img.local && img.persistent)
		{
			String cmd = img.state.extract_txt("cmdline");
			System.out.println("Attempting to restart persistent component:");
			System.out.printf("   %s\n", cmd);
			persist.start(cmd);
		}
	}

	void lookup(Endpoint ep)
	{
		ProtoMessage query;
		Node result, sn_constraints, sn_interface;
		Image img;
		int count = 0;

		query = ep.rcv();
		// @criteria { ^map-constraints - ^interface - }
		sn_constraints = query.tree.extract_item("map-constraints");
		sn_interface = query.tree.extract_item("interface");
		result = Node.mklist("results");
		// @results ( txt address )
		live_mutex.lock();
		for(int i = 0; i < live.size(); i++)
		{
			img = live.get(i);
			if(img.match(sn_interface, sn_constraints, com))
			{
				result.append(Node.pack(img.address, "address"));
				count++;
			}
		}
		live_mutex.unlock();
		System.out.println("lookup component returning " + count + " matches");
		ep.reply(query, result);
	}
	
	void list(Endpoint ep)
	{
		ProtoMessage query;
		Node result, sn;
		Image img;
		String cpt_name, instance;

		query = ep.rcv();

		result = Node.mklist("cpt-list");
		live_mutex.lock();
		for(int i = 0; i < live.size(); i++)
		{
			img = live.get(i);
			if(img.metadata == null || img.state == null)
				continue; // No info yet

			sn = Node.mklist("component");
			cpt_name = img.metadata.extract_txt("name");
			instance = img.state.extract_txt("instance");
			sn.append(Node.pack(img.address, "address"));
			sn.append(Node.pack(cpt_name, "cpt-name"));
			if(instance.equals(cpt_name))
				sn.append(Node.pack_empty("instance"));
			else
				sn.append(Node.pack(instance, "instance"));

			result.append(sn);
		}
		live_mutex.unlock();
		ep.reply(query, result);
	}
	
	void dump(Endpoint ep)
	{
		ProtoMessage query;
		Node result, sn;
		Image img;

		query = ep.rcv();

		result = Node.mklist("rdc-dump");
		live_mutex.lock();
		for(int i = 0; i < live.size(); i++)
		{
			img = live.get(i);
			if(img.metadata == null || img.state == null)
				continue; // No info yet
			sn = Node.mklist("component");

			sn.append(Node.pack(img.address, "address"));
			sn.append(new Node(img.metadata, "metadata"));
			sn.append(new Node(img.state, "state"));

			result.append(sn);
		}
		live_mutex.unlock();
		ep.reply(query, result);
	}

	void remotestart(Endpoint ep)
	{
		ProtoMessage msg;
		String cmdline;

		msg = ep.rcv();
		cmdline = msg.tree.extract_txt();
		System.out.printf("Remote starting <%s>\n", cmdline);
		persist.start(cmdline);
	}

	boolean is_local(String address)
	{
		int pos;

		pos = address.indexOf(':');
		if(pos == -1 || pos == 0)
			Log.error("Invalid address passed to is_local()");
		if(address.substring(0, pos).equals(local_address))
			return true;
		return false;
	}

	private void read_local_address()
	{
		String addr;
		int pos;

		addr = com.get_address();
		pos = addr.indexOf(':');
		if(pos == -1 || pos == 0)
			Log.error("Invalid address returned by scomponent::get_address()");
		local_address = addr.substring(0, pos);
	}
	
	// Only call this from the master thread, since it may call clone():
	private int first_free_pool()
	{
		int pool_id;

		pool_mutex.lock();
		try
		{
			for(int i = 0; i < pool_busy.size(); i++)
			{
				if(pool_busy.get(i) == false)
				{
					pool_busy.set(i, true);
					return i;
				}
			}
			if(pool_busy.size() == max_pool_size)
			{
				System.out.println("Serious warning: maximum thread pool size " +
						"reached; registrations not being processed\n");
				return -1;
			}
			pool_busy.add(true);
			metadata_epv.add(com.clone(metadata_ep));
			status_epv.add(com.clone(status_ep));
			pool_id = pool_busy.size() - 1;
		}
		finally
		{
			pool_mutex.unlock();
		}
		return pool_id;
	}

	static void release_pool(int pool_id, Lock pool_mutex,
			ArrayList<Boolean> pool_busy)
	{
		pool_mutex.lock();
		pool_busy.set(pool_id, false);
		pool_mutex.unlock();
	}
};

class RegisterBack implements Runnable
{
	RDC obj;
	int pool_id;
	Image img;
	
	RegisterBack(RDC obj, int pool_id, Image img)
	{
		this.obj = obj;
		this.pool_id = pool_id;
		this.img = img;
	}
	
	/* registerback could block in status_ep or metadata_ep, so separate
		threads run it: */
	
	public void run()
	{
		ProtoMessage info;
		String addr;
		Persistence persist;
		Endpoint status_ep, metadata_ep;
		Lock live_mutex;
		ArrayList<Image> live;

		try
		{
			persist = obj.persist;
			addr = img.address;
			Log.debug("registerback using pool slot " + pool_id);
			status_ep = obj.status_epv.get(pool_id);
			metadata_ep = obj.metadata_epv.get(pool_id);
			live_mutex = obj.live_mutex;
			live = obj.live;

			if(metadata_ep.map(addr, "get_metadata") == null)
			{
				System.out.printf("Warning: RDC could not connect back to " +
						"registering component at '%s'.\n", addr);
				return;
			}
			info = metadata_ep.rpc(null);
			if(info == null)
			{
				System.out.printf("Warning: RDC could not fetch metadata for " +
						"registering component %s\n", addr);
				metadata_ep.unmap();
				return;
			}
			img.metadata = info.tree;
			metadata_ep.unmap();

			if(status_ep.map(addr, "get_status") == null)
			{
				System.out.println("Warning: RDC could not connect back to " +
						"registering component.");
				return;
			}
			info = status_ep.rpc(null);
			if(info == null)
			{
				System.out.printf("Warning: RDC could not fetch status for " +
						"registering component %s\n", addr);
				status_ep.unmap();
				return;
			}
			img.state = info.tree;
			status_ep.unmap();

			img.tv_ping = System.currentTimeMillis();

			img.init_hashes(obj.com);
			live_mutex.lock();
			live.add(img);
			live_mutex.unlock();

			String cpt_name = img.metadata.extract_txt("name");
			String cpt_instance = img.state.extract_txt("instance");
			if(cpt_name.equals(cpt_instance))
			{
				System.out.printf("Registered component %s at %s\n",
						cpt_name, img.address);
			}
			else
			{
				System.out.printf("Registered component %s, instance %s, at %s\n",
						cpt_name, cpt_instance, img.address);
			}
			System.out.printf("Command line was \"%s\"\n",
					img.state.extract_txt("cmdline"));
			if(img.local &&
					persist.is_persistent(img.state.extract_txt("cmdline")))
			{
				img.persistent = true;
			}
			System.out.printf("local = %b, persistent = %b\n",
					img.local, img.persistent);
		}
		finally
		{
			RDC.release_pool(pool_id, obj.pool_mutex, obj.pool_busy);
		}
	}
};

class CheckAlive implements Runnable
{
	RDC obj;
	int pool_id;
	
	CheckAlive(RDC obj, int pool_id)
	{
		this.obj = obj;
		this.pool_id = pool_id;
	}
	
	// checkalive could block in status_ep rpc, so separate threads run it:
	
	public void run()
	{
		Image img, imgx;
		long tv_oldest;
		ProtoMessage info;
		Endpoint status_ep, metadata_ep;
		Lock live_mutex;
		ArrayList<Image> live;
		String location;

		try
		{
			status_ep = obj.status_epv.get(pool_id);
			metadata_ep = obj.metadata_epv.get(pool_id);
			live_mutex = obj.live_mutex;
			live = obj.live;

			live_mutex.lock();
			if(live.size() == 0)
			{
				live_mutex.unlock();
				return;
			}

			// Find the image least recently checked:
			img = live.get(0);
			tv_oldest = img.tv_ping;
			for(int i = 0; i < live.size(); i++)
			{
				imgx = live.get(i);
				if(imgx.tv_ping < tv_oldest)
				{
					img = imgx;
					tv_oldest = imgx.tv_ping;
				}
			}
			live_mutex.unlock();

			String cpt_name = img.metadata.extract_txt("name");
			String cpt_instance = img.state.extract_txt("instance");
			if(cpt_instance != null && !cpt_name.equals(cpt_instance))
			{
				System.out.printf("Checking component %s(%s) %s still live\n",
						cpt_name, cpt_instance, img.address);
			}
			else
			{
				System.out.printf("Checking component %s at %s still live\n",
						cpt_name, img.address);
			}

			// Ping it by asking for new status:
			location = status_ep.map(img.address, "get_status");
			if(location == null)
			{
				System.out.printf("Ping indicates component %s at %s vanished\n" +
						"   without deregistering; removing it from list\n",
						img.metadata.extract_txt("name"), img.address);
				disconnect(img);
				return;
			}
			info = status_ep.rpc(null);
			if(info == null)
			{
				System.out.println("Component disconnected during ping (unusual); " +
						"removing it from list");
				disconnect(img);
				return;
			}

			// Update status:
			img.state = info.tree;
			info.tree = null;
			status_ep.unmap();

			// Update its ping time:
			img.tv_ping = System.currentTimeMillis();

			// Ask for new metadata:
			location = metadata_ep.map(img.address, "get_metadata");
			if(location == null)
			{
				System.out.printf("Ping indicates component %s at %s vanished\n" +
						"   without deregistering; removing it from list\n",
						img.metadata.extract_txt("name"), img.address);
				disconnect(img);
				return;
			}
			info = metadata_ep.rpc(null);
			if(info == null)
			{
				System.out.println("Component disconnected during get_metadata " +
						"(unusual); removing it from list\n");
				disconnect(img);
				return;
			}
			// Update metadata:
			img.metadata = info.tree;
			metadata_ep.unmap();
		}
		finally
		{
			RDC.release_pool(pool_id, obj.pool_mutex, obj.pool_busy);
		}
	}
	
	void disconnect(Image img)
	{
		Lock live_mutex = obj.live_mutex;
		
		live_mutex.lock();
		obj.live.remove(img);
		live_mutex.unlock();
		if(img.local && img.persistent)
		{
			String cmd = img.state.extract_txt("cmdline");
			System.out.println("Attempting to restart persistent component:");
			System.out.printf("   %s\n", cmd);
			obj.persist.start(cmd);
		}
	}
};
