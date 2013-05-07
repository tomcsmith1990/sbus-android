import java.util.*;
import java.nio.channels.*;

/* ProtoMessage is the structure used for all non-error messages passed to
	the application from the wrapper, via the library: */

class ProtoMessage extends Protocol
{
	ProtoMessage()
	{
		source_cpt = source_inst = source_ep = null;
		topic = null;
		source_ep_id = 0;
		seq = 0;
		hc = null;
		xml = null;
		tree = null;
		reason = 0;
		oob_returncode = null;
	}

	int reveal(AbstractMessage abst) // -1 = bad msg type, -2 = bad xml, 0 = OK
	{
		ByteDecoder bd;

		type = abst.get_type();
		if(type != MessageType.Rcv && type != MessageType.Response &&
				type != MessageType.Unavailable && type != MessageType.ReturnCode)
		{
			return -1;
		}

		bd = new ByteDecoder(abst.get_data());
		if(type == MessageType.Unavailable)
		{
			reason = bd.decode_byte();
		}
		else if(type == MessageType.ReturnCode)
		{
			oob_returncode = new ProtoReturnCode();
			oob_returncode.retcode = bd.decode_count();
			oob_returncode.address = bd.decode_string();
		}
		else
		{
			source_cpt = bd.decode_string();
			source_inst = bd.decode_string();
			source_ep = bd.decode_string();
			topic = bd.decode_string();
			source_ep_id = bd.decode_count();
			seq = bd.decode_count();
			hc = bd.decode_hashcode();

			xml = new String(bd.tail());
			// Fill in tree:
			try
			{
				tree = XML.fromxml(xml);
			}
			catch(ImportException e)
			{
				// "Error converting payload from XML: " + e.msg
				return -2;
			}
		}
		return 0;
	}
	
	AbstractMessage wrap(SocketChannel sc)
	{
		ByteEncoder be;
		AbstractMessage abst;

		// Convert payload to XML:
		if(xml == null && tree != null)
			xml = XML.toxml(tree, false);

		be = begin_msg(type);
		if(type == MessageType.Unavailable)
		{
			be.cat_byte(reason);
		}
		else
		{
			be.cat_string(source_cpt);
			be.cat_string(source_inst);
			be.cat_string(source_ep);
			be.cat_string(topic);
			be.cat(source_ep_id);
			be.cat(seq);
			be.cat(hc);
			be.cat(xml);
		}
		abst = new AbstractMessage(sc, be);
		return abst;
	}
						
	MessageType type; // MessageType: Rcv, Response, Unavailable;
	                  // or OOB type ReturnCode
	String source_cpt;
	String source_inst;
	String source_ep;
	int source_ep_id;
	String topic; // null if none specified, or for RPCs
	int seq;
	HashCode hc;
	Node tree;
	Object state; // For conversations only - not implemented yet
	
	int reason; // MessageType.Unavailable only

	// Out of bound alternative (if set, overrides rest of this class):
	ProtoReturnCode oob_returncode;
	
	// Managed internally by smessage - callers just set/get tree:
	private String xml;
};

class ProtoControl extends Protocol
{
	ProtoControl()
	{
		address = target_endpoint = null;
		subs = topic = peer = null;
	}

	int reveal(AbstractMessage abst) // returns -1 if wrong msg type, else 0
	{
		ByteDecoder bd;

		type = abst.get_type();
		if(type != MessageType.Map && type != MessageType.Unmap &&
				type != MessageType.Ismap && type != MessageType.Subscribe)
		{
			return -1;
		}

		bd = new ByteDecoder(abst.get_data());
		if(type == MessageType.Subscribe)
		{
			subs = bd.decode_string();
			topic = bd.decode_string();
			peer = bd.decode_string();
		}
		else
		{
			address = bd.decode_string();
			target_endpoint = bd.decode_string();
		}
		return 0;
	}

	AbstractMessage wrap(SocketChannel sc)
	{
		ByteEncoder be;
		AbstractMessage abst;

		be = begin_msg(type);
		if(type == MessageType.Subscribe)
		{
			be.cat_string(subs); // May be null
			be.cat_string(topic); // May be null
			be.cat_string(peer); // May be null
		}
		else
		{
			be.cat_string(address); // May be null
			be.cat_string(target_endpoint); // May be null
		}
		abst = new AbstractMessage(sc, be);
		return abst;
	}
				
	MessageType type; // MessageType: Map, Unmap, Ismap or Subscribe
	
	// For the three mapping calls:
	String address;
	String target_endpoint;
	
	// For MessageType.Subscribe:
	String subs;
	String topic;
	String peer;
};

class ProtoRunning extends Protocol
{
	ProtoRunning()
	{
		builtins = null;
		address = null;
		xml = null;
	}
		
	int reveal(AbstractMessage abst) // returns -1 if wrong msg type, else 0
	{
		ByteDecoder bd;
		MessageType type;

		type = abst.get_type();	
		if(type != MessageType.Running)
			return -1;

		bd = new ByteDecoder(abst.get_data());

		listen_port = bd.decode_count();
		address = bd.decode_string();
		xml = new String(bd.tail());

		// Fill in builtins tree:
		try
		{
			builtins = XML.fromxml(xml);
		}
		catch(ImportException e)
		{
			Log.debug("Error converting payload from XML: " + e.msg);
			return -2;
		}

		return 0;
	}

	AbstractMessage wrap(SocketChannel sc)
	{
		ByteEncoder be;
		AbstractMessage abst;

		// Convert payload to XML:
		if(xml == null && builtins != null)
			xml = XML.toxml(builtins, false);
		if(xml == null)
			Log.error("Failed sanity check in ProtoRunning.wrap()");
		
		be = begin_msg(MessageType.Running);
		be.cat(listen_port);
		be.cat_string(address);
		be.cat(xml);

		abst = new AbstractMessage(sc, be);
		return abst;
	}
	
	int listen_port;
	String address;
	Node builtins;
	
	// Managed internally by class - callers just set/get builtins:
	private String xml;
};

class ProtoReturnCode extends Protocol
{
	ProtoReturnCode()
	{
		address = null;
	}

	int reveal(AbstractMessage abst) // returns -1 if wrong msg type, else 0
	{
		ByteDecoder bd;

		if(abst.get_type() != MessageType.ReturnCode)
			return -1;

		bd = new ByteDecoder(abst.get_data());
		retcode = bd.decode_count();
		address = bd.decode_string();

		return 0;
	}

	AbstractMessage wrap(SocketChannel sc)
	{
		ByteEncoder be;
		AbstractMessage abst;

		be = begin_msg(MessageType.ReturnCode);
		be.cat(retcode);
		be.cat_string(address);

		abst = new AbstractMessage(sc, be);
		return abst;
	}
	
	int retcode;
	String address;
};

class ProtoStopWrapper extends Protocol
{
	int reveal(AbstractMessage abst) // returns -1 if wrong msg type, else 0
	{
		ByteDecoder bd;
		MessageType type;

		type = abst.get_type();	
		if(type != MessageType.Stop)
			return -1;

		bd = new ByteDecoder(abst.get_data());
		reason = bd.decode_count();

		return 0;
	}
	
	AbstractMessage wrap(SocketChannel sc)
	{
		ByteEncoder be;
		AbstractMessage abst;

		be = begin_msg(MessageType.Stop);
		be.cat(reason);
		abst = new AbstractMessage(sc, be);
		return abst;
	}
	
	int reason;
};

class ProtoHook extends Protocol
{
	ProtoHook(MessageType t)
	{
		hc = null;
		tree = null;
		type = t;
	}

	ProtoHook()
	{
		hc = null;
		tree = null;
		type = MessageType.Unknown;
	}

	int reveal(AbstractMessage abst) // returns -1 if wrong msg type, else 0
	{
		ByteDecoder bd;
		String xml;

		type = abst.get_type();	
		if(type != MessageType.GetStatus && type != MessageType.GetSchema &&
				type != MessageType.Declare)
			return -1;

		tree = null;
		bd = new ByteDecoder(abst.get_data());
		hc = bd.decode_hashcode();
		if(bd.eof())
			return 0; // tree remains null
		
		xml = new String(bd.tail());
		// Fill in tree:
		try
		{
			tree = XML.fromxml(xml);
		}
		catch(ImportException e)
		{
			// "Error converting payload from XML: " + e.msg
			return -2;
		}
		return 0;	
	}

	AbstractMessage wrap(SocketChannel sc)
	{
		ByteEncoder be;
		AbstractMessage abst;
		String xml = null;

		if(hc == null)
		{
			hc = new HashCode();
			hc.frommeta(MetaType.SCHEMA_EMPTY);
		}
		if(tree != null)
			xml = XML.toxml(tree, false);

		be = begin_msg(type);
		be.cat(hc);
		if(tree != null)
			be.cat(xml);
		abst = new AbstractMessage(sc, be);
		return abst;
	}
	
	MessageType type; // MessageType: Status, GetSchema, Declare
	
	HashCode hc;	
	Node tree;
		
	/* MessageType.GetStatus: 0         - 000000000000
		MessageType.GetSchema: @txt hash - D3C74D1897A3
		MessageType.Declare: @declare { txt schema flg file_lookup }
                                     - 3D79D04FEBCC */
};

class ProtoGeneric extends Protocol
{
	ProtoGeneric(MessageType t)
	{
		hc = null;
		tree = null;
		type = t;
	}

	ProtoGeneric()
	{
		hc = null;
		tree = null;
		type = MessageType.Unknown;
	}

	int reveal(AbstractMessage abst) // returns -1 if wrong msg type, else 0
	{
		ByteDecoder bd;
		String xml;

		type = abst.get_type();	
		if(type != MessageType.Status && type != MessageType.Schema &&
				type != MessageType.Hash)
			return -1;

		bd = new ByteDecoder(abst.get_data());
		hc = bd.decode_hashcode();
		xml = new String(bd.tail());

		// Fill in tree:
		try
		{
			tree = XML.fromxml(xml);
		}
		catch(ImportException e)
		{
			// "Error converting payload from XML: " + e.msg
			return -2;
		}

		return 0;
	}

	AbstractMessage wrap(SocketChannel sc)
	{
		ByteEncoder be;
		AbstractMessage abst;
		String xml;

		xml = XML.toxml(tree, false);	
		be = begin_msg(type);
		be.cat(hc);
		be.cat(xml);
		abst = new AbstractMessage(sc, be);
		return abst;
	}

	MessageType type; // MessageType: Status, Schema, Hash
	
	HashCode hc;	
	Node tree;
	
	/* MessageType.Status: ^component-state
		MessageType.Schema: @txt schema - D39E44946A6C
		MessageType.Hash:   @txt hash - D3C74D1897A3 */
};

class ProtoAddEndpoint extends Protocol // MessageType.AddEndpoint
{
	ProtoAddEndpoint()
	{
		endpoint = null;
		msg_hc = reply_hc = null;
	}

	void clear()
	{
		endpoint = null;
		msg_hc = reply_hc = null;
	}
	
	AbstractMessage wrap(SocketChannel sc)
	{
		ByteEncoder be;
		AbstractMessage abst;

		be = begin_msg(MessageType.AddEndpoint);
		be.cat_string(endpoint);
		be.cat_byte(type.ordinal());
		be.cat(msg_hc);
		be.cat(reply_hc);
		abst = new AbstractMessage(sc, be);
		return abst;
	}
	
	int reveal(AbstractMessage abst) // returns -1 if wrong msg type, else 0
	{
		// Dummy method: not actually used, but lets us be a Protocol subclass
		return 0;
	}
	
	String endpoint;
	EndpointType type;
	HashCode msg_hc;
	HashCode reply_hc;
};

class ProtoStartWrapper extends Protocol
{
	ProtoStartWrapper()
	{
		cpt_name = instance_name = creator = metadata_address = null;
		rdc = new Vector<String>();
		unique = CptUniqueness.Multiple;
	}

	AbstractMessage wrap(SocketChannel sc)
	{
		ByteEncoder be;
		AbstractMessage abst;
		String s;

		be = begin_msg(MessageType.Start);
		be.cat_string(cpt_name);
		be.cat_string(instance_name);
		be.cat_string(creator);
		be.cat_string(metadata_address); // May be null
		be.cat(listen_port);
		be.cat_byte(unique.ordinal());
		be.cat_byte(log_level);
		be.cat_byte(echo_level);

		// RDCs:
		be.cat(rdc.size());
		for(int i = 0; i < rdc.size(); i++)
		{
			s = rdc.get(i);
			be.cat_string(s);
		}

		abst = new AbstractMessage(sc, be);
		return abst;
	}

	int reveal(AbstractMessage abst) // returns -1 if wrong msg type, else 0
	{
		// Dummy method: not actually used, but lets us be a Protocol subclass
		return 0;
	}
	
	String cpt_name;
	String instance_name;
	String creator;
	String metadata_address;
	int listen_port;
	CptUniqueness unique;
	int log_level;
	int echo_level;
	Vector<String> rdc;
};

/* ProtoInternal is the general structure for all messages passed from the
	library to the wrapper: */

class ProtoInternal extends Protocol
{
	ProtoInternal()
	{
		topic = null;
		hc = null;
		xml = null;
		seq = 0;

		oob_ctrl = null;
	}

	int reveal(AbstractMessage abst) // returns -1 if wrong msg type, else 0
	{
		ByteDecoder bd;

		type = abst.get_type();
		if(type != MessageType.Reply && type != MessageType.Emit &&
				type != MessageType.RPC && type != MessageType.Map &&
				type != MessageType.Unmap && type != MessageType.Ismap &&
				type != MessageType.Subscribe)
			return -1;

		bd = new ByteDecoder(abst.get_data());

		if(type == MessageType.Subscribe)
		{
			oob_ctrl = new ProtoControl();
			oob_ctrl.subs = bd.decode_string();
			oob_ctrl.topic = bd.decode_string();
			oob_ctrl.peer = bd.decode_string();
		}
		else if(type == MessageType.Map || type == MessageType.Unmap ||
			type == MessageType.Ismap)
		{
			oob_ctrl = new ProtoControl();
			oob_ctrl.address = bd.decode_string();
			oob_ctrl.target_endpoint = bd.decode_string();
		}
		else // MessageType: Reply, Emit, RPC
		{
			// Normal data message from library to wrapper:

			topic = bd.decode_string();
			seq = bd.decode_count();
			hc = bd.decode_hashcode();
			xml = new String(bd.tail());
		}
		return 0;
	}

	AbstractMessage wrap(SocketChannel sc)
	{
		ByteEncoder be;
		AbstractMessage abst;

		be = begin_msg(type);
		if(type == MessageType.Emit)
			be.cat_string(topic); // May be null
		else
			be.cat(0); // Empty topic string
		be.cat(seq);
		be.cat(hc);
		be.cat(xml);
		abst = new AbstractMessage(sc, be);
		return abst;
	}

	MessageType type; // MessageType: Emit, RPC, Reply
	                  // Or OOB types Map, Unmap, Ismap, Subscribe
	String topic;     // MessageType.Emit only
	int seq;          // MessageType.Reply only
	HashCode hc;
	String xml;
	
	// Out of bound messages (if set, overrides rest of this class):
	ProtoControl oob_ctrl;
};

class ProtoMulti
{
	/* read_bootupdate() returns:
		MessageType.AddEndpoint = filled add,
		MessageType.Stop        = filled stop,
		MessageType.GetStatus   = filled hook,
		MessageType.GetSchema   = filled hook,
		MessageType.Declare     = filled hook,
		-1                      = disconnect,
		-2                      = none of these message types matches */

	int read_bootupdate(AbstractMessage abst, ProtoAddEndpoint add,
			ProtoStopWrapper stop, ProtoHook hook)
	{
		ByteDecoder bd;
		MessageType t;

		t = abst.get_type();
		bd = new ByteDecoder(abst.get_data());

		if(t == MessageType.AddEndpoint)
		{
			add.endpoint = bd.decode_string();
			add.type = EndpointType.values()[bd.decode_byte()];
			add.msg_hc = bd.decode_hashcode();
			add.reply_hc = bd.decode_hashcode();
		}
		else if(t == MessageType.Stop)
		{
			stop.reason = bd.decode_count();
		}
		else if(t == MessageType.GetStatus || t == MessageType.GetSchema ||
			t == MessageType.Declare)
		{
			String xml;

			hook.type = t;
			hook.hc = bd.decode_hashcode();
			if(!bd.eof())
			{
				xml = new String(bd.tail());
				try
				{
					hook.tree = XML.fromxml(xml);
				}
				catch(ImportException e)
				{
					return -2;
				}
			}
		}
		else
		{
			return -2;
		}

		return t.ordinal();
	}

	int read_bootupdate(SocketChannel sc, ProtoAddEndpoint add,
			ProtoStopWrapper stop, ProtoHook hook)
	{
		AbstractMessage abst;
		int ret;

		abst = new AbstractMessage(sc);
		ret = abst.blockadvance();
		if(ret < 0) // Disconnect (-1) or protocol error (-2)
			return ret;
		return read_bootupdate(abst, add, stop, hook);
	}

	/* read_startup() returns 0 = filled add, 1 = filled start,
		-1 = neither type matches: */
	
	int read_startup(AbstractMessage abst, ProtoAddEndpoint add,
			ProtoStartWrapper start)
	{
		ByteDecoder bd;
		MessageType t;

		t = abst.get_type();
		if(t != MessageType.AddEndpoint && t != MessageType.Start)
			return -1;

		bd = new ByteDecoder(abst.get_data());
		if(t == MessageType.AddEndpoint)
		{
			add.endpoint = bd.decode_string();
			add.type = EndpointType.values()[bd.decode_byte()];
			add.msg_hc = bd.decode_hashcode();
			add.reply_hc = bd.decode_hashcode();
			return 0;
		}
		else // MessageType.Start
		{
			int num_rdc;
			String s;

			start.cpt_name = bd.decode_string();
			start.instance_name = bd.decode_string();
			start.creator = bd.decode_string();
			start.metadata_address = bd.decode_string();
			start.listen_port = bd.decode_count();
			start.unique = CptUniqueness.values()[bd.decode_byte()];
			start.log_level = bd.decode_byte();
			start.echo_level = bd.decode_byte();
			num_rdc = bd.decode_count();
			start.rdc.clear();
			for(int i = 0; i < num_rdc; i++)
			{
				s = bd.decode_string();
				start.rdc.add(s);
			}
			return 1;
		}	
	}

	/* read_startup() returns 0 = filled add, 1 = filled start,
		-1 = disconnect, -2 = bad protocol: */
	
	int read_startup(SocketChannel sc, ProtoAddEndpoint add,
			ProtoStartWrapper start)
	{
		AbstractMessage abst;
		int ret;

		abst = new AbstractMessage(sc);
		ret = abst.blockadvance();
		if(ret < 0) // Disconnect (-1) or protocol error (-2)
			return ret;
		return read_startup(abst, add, start);
	}
};
