import java.io.*;
import java.util.*;
import java.nio.*;
import java.nio.channels.*;

enum MessageType
{
	// Component to component:
	Hello, Welcome,               // Remote connection setup
	Sink, Server, Client,         // Remote data transfer
	Flow,           // Flow control, sent from sink to source
	Goodbye, Resubscribe, Divert, // Out-of-band messages
	Visitor,                      // Disposable connections
	
	// Library to wrapper:
	AddEndpoint, Start, // Bootstrap pipe setup
	Stop,               // Bootstrap pipe runtime/close
	Running,            // Bootstrap pipe reply
	GetStatus, Status,  // Bootstrap pipe runtime
	GetSchema, Schema,  // Bootstrap pipe runtime
	Declare, Hash,      // Bootstrap pipe runtime
	Map, Unmap, Ismap,  // Local mapping control messages
	ReturnCode,         // Mapping control reply
	Subscribe,          // Local control message
	Emit, RPC, Reply,   // Library to wrapper
	Rcv, Response, Unavailable, // Wrapper to library
	
	Unknown // Internal use
};

enum AcceptanceCode
{
	AcceptOK,
	AcceptWrongCpt,
	AcceptNoEndpoint,
	AcceptWrongType,
	AcceptNotCompatible,
	AcceptNoAccess,
	AcceptProtocolError,
	AcceptAlreadyMapped;
			
	String str[] =
	{
		"AcceptOK",
		"AcceptWrongCpt",
		"AcceptNoEndpoint",
		"AcceptWrongType",
		"AcceptNotCompatible",
		"AcceptNoAccess",
		"AcceptProtocolError",
		"AcceptAlreadyMapped"
	};
};

enum MessageState
{
	Header,
	Body,
	Done
};

enum VisitPurpose
{
	NonVisit,
	VisitRegister,
	VisitLookupSchema,
	VisitResolveConstraints,
	VisitLost
};
	
class AbstractMessage
{
	static final int HEADER_LENGTH = 10;
	
	// Outbound (sb has type in header):
	AbstractMessage(SocketChannel sc, ByteEncoder be)
	{
		this.sc = sc;
		header = null;
		length = be.length();
		be.overwrite_word(5, length); // Patch in message length
		body = be.extract();
		bb = ByteBuffer.wrap(body);
		state = MessageState.Body;
		filled = 0;
		outbound = true;
		diverting = 0;
		address = null;
		purpose = VisitPurpose.NonVisit;
	}

	// Inbound:
	AbstractMessage(SocketChannel sc)
	{
		this.sc = sc;
		header = new byte[10];
		bb = ByteBuffer.wrap(header);
		body = null;
		state = MessageState.Header;
		filled = 0;
		outbound = false;
		diverting = 0;
		address = null;
		purpose = VisitPurpose.NonVisit;
	}

	/* Returns 1 if complete, 0 if incomplete, -1 on disconnect
		or -2 on protocol error (latter for inbound messages only): */
	int advance()
	{
		return do_advance(false, 0);
	}

	int blockadvance() // Like advance(), but never returns 0 and may block
	{
		// Returns 1 if complete, -1 on disconnect or -2 on protocol error
		return do_advance(true, 0);
	}
		
	int blockadvance(int timeout_usec) // As above, but returns 0 if timeout
	{
		/* Returns 1 if complete, -1 on disconnect, -2 on protocol error,
			or 0 on timeout */
		return do_advance(true, timeout_usec);
	}
	
	int partial_advance() // For testing purposes on writes only
	{
		return do_advance(false, 0, 4); // Just write a few (4) bytes
	}

	byte[] get_data()
	{
		return body;
	}

	int get_length() // length of data (body)
	{
		return (length - HEADER_LENGTH);
	}

	MessageType get_type()
	{
		return type;
	}
	
	/* Protocol-specific state needed to complete an operation; carried along
		for the ride in AbstractMessage (not our responsibility to free): */

	/*
	speer *peer;           // FDWelcoming, FDGreeting and FDPending
	int report_fd;         // FDGreeting and FDPending
	*/
	int diverting;         // FDPeer

	/* Protocol-specific state needed for internal visitor connections: */
		
	VisitPurpose purpose;  // Visitor connections only, else NonVisit
	/*
	Schema *reply_schema;  // FDVisiting and FDDisposing
	mapparams *map_params; // FDVisiting and FDDisposing (resolving constraints)
	registerparams *reg_params; // Likewise (registering or deregistering)
	scomm *unrecognised;   // FDVisiting and FDDisposing (schema lookups)
	int terminate;         // FDVisiting
	*/
	String address;        // FDConnecting

	/* End of protocol-specific passenger state */
	
	private byte[] header; // Only considered separately for inbound
	private byte[] body;
	ByteBuffer bb;
	private int length; // Total length (header + body)
	private MessageType type;
	
	private SocketChannel sc;
	private boolean outbound;
	private int filled; // bytes processed in this phase (header/body) so far
	private MessageState state;

	// Waits (possibly forever) for fd to become ready:
	private int ready()
	{
		return ready(0);
	}
	
	private int ready(int timeout_usec)
	{
		// Waits for sc to become ready, in appropriate direction
		// Returns 0 on timeout, 1 if ready, -1 if disconnected
		// timeout_usec may be 0
		
		Selector selector = null;
		SelectionKey key;
		int keys;

		try { selector = Selector.open(); }
		catch(IOException e) { Log.error("Can't open a selector"); }
		
		try
		{
			if(outbound)
				key = sc.register(selector, SelectionKey.OP_WRITE);
			else
				key = sc.register(selector, SelectionKey.OP_READ);
		}
		catch(ClosedChannelException e)
		{
			return -1;
		}

		if(timeout_usec == 0)
		{
			try { selector.select(); }
			catch(Exception e) { return -1; }
			return 1;
		}
		try
		{
			keys = selector.select(timeout_usec / 1000);
			if(keys > 0)
				return 1;
			else
				return 0;
		}
		catch(Exception e) { return -1; }
	}
	
	private int do_advance(boolean block, int timeout_usec)
	{
		return do_advance(block, timeout_usec, -1);
	}
	
	private int do_advance(boolean block, int timeout_usec, int max_bytes)
	{
		// Returns 1 if complete, 0 if incomplete (and non-blocking),
		// -1 on disconnect or -2 on protocol error
		int bytes;
		int target;
		int ret;

		if(state == MessageState.Done)
			Log.error("Tried to advance a completed message");
		if(outbound)
		{
			while(filled < length && (max_bytes == -1 || filled < max_bytes))
			{
				target = length - filled;
				if(max_bytes != -1 && target > max_bytes) target = max_bytes;
				try { bytes = sc.write(bb); }
				catch(IOException e)
				{
					Log.warning("Outbound message error (length = " + length +
							", filled = " + filled + "): " + e);
					return -1;
				}
				if(bytes == 0)
				{
					if(block)
					{
						ret = ready(timeout_usec);
						if(ret <= 0)
							return ret;
						continue;
					}
					else
						return 0;
				}
				filled += bytes;
			}
			if(filled == length)
				state = MessageState.Done;
		}
		else
		{
			// Inbound
			if(state == MessageState.Header)
			{
				while(filled < HEADER_LENGTH)
				{
					try { bytes = sc.read(bb); }
					catch(IOException e)
					{
						Log.warning("Inbound message error");
						return -1;
					}
					if(bytes == 0)
					{
						if(block)
						{
							ret = ready(timeout_usec);
							if(ret <= 0)
								return ret;
							continue;
						}
						else
							return 0;
					}
					filled += bytes;
				}
				
				// Parse header:
				byte[] magic = Protocol.MAGIC_STRING.getBytes();
				for(int i = 0; i < 4; i++)
					if(header[i] != magic[i])
						return -2; // Incorrect magic number
				if(header[4] != Protocol.PROTOCOL_VERSION)
					return -2; // Incorrect protocol version
				length = (Utils.btoi(header[5]) << 24) |
						(Utils.btoi(header[6]) << 16) |
						(Utils.btoi(header[7]) << 8) | Utils.btoi(header[8]);
				type = MessageType.values()[header[9]];
				state = MessageState.Body;
				body = new byte[length - HEADER_LENGTH];
				bb = ByteBuffer.wrap(body);
				filled = 0;
			}
			
			// MessageState.Body
			while(filled < length - HEADER_LENGTH)
			{
				try { bytes = sc.read(bb); }
				catch(IOException e)
				{
					Log.warning("Inbound message error");
					return -1;
				}
				if(bytes == 0)
				{
					if(block)
					{
						ret = ready(timeout_usec);
						if(ret <= 0)
							return ret;
						continue;
					}
					else
						return 0;
				}
				filled += bytes;
			}
			state = MessageState.Done;
		}
		return 1;
	}
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

abstract class Protocol
{
	static final byte PROTOCOL_VERSION = 8;
	static final String MAGIC_STRING = "sBuS";

	int read(SocketChannel sc) // -1 on disconnect, -2 on bad protocol, else 0
	{
		AbstractMessage abst;
		int ret;

		abst = new AbstractMessage(sc);
		ret = abst.blockadvance();
		if(ret < 0) // Disconnect (-1) or protocol error (-2)
			return ret;
		ret = reveal(abst);
		if(ret < 0) // Wrong msg type (-1)
			return -2;
		return 0;
	}

	int write(SocketChannel sc) // -1 if disconnected, else 0
	{
		AbstractMessage abst;
		int ret;

		abst = wrap(sc);
		ret = abst.blockadvance();
		if(ret == 1) // Completed OK
			return 0;
		return ret;
	}
	
	abstract int reveal(AbstractMessage abst); // -1 wrong msg type, else 0
		// reveal() may also return -2 for bad XML! - XXXX
	abstract AbstractMessage wrap(SocketChannel sc);

	static ByteEncoder begin_msg(MessageType type)
	{
		ByteEncoder be;

		be = new ByteEncoder();
		be.cat(MAGIC_STRING);
		be.cat_byte(PROTOCOL_VERSION);
		be.skip(4); // Placeholder for message length
		be.cat_byte(type.ordinal());
		return be;
	}

	// Returns message length, or -1 on disconnect or -2 on protocol error:
	static int read_header(SocketChannel sc, MessageType type)
	{
		byte[] buf, magic;
		int msg_length;

		buf = new byte[AbstractMessage.HEADER_LENGTH];
		if(LowLevel.fixed_read(sc, buf, AbstractMessage.HEADER_LENGTH) == false)
			return -1;

		magic = MAGIC_STRING.getBytes();
		for(int i = 0; i < 4; i++)
			if(buf[i] != magic[i])
				return -2; // "Incorrect magic number"
		if(buf[4] != PROTOCOL_VERSION)
			return -2; // "Incorrect protocol version"
		msg_length = (Utils.btoi(buf[5]) << 24) | (Utils.btoi(buf[6]) << 16) |
				(Utils.btoi(buf[7]) << 8) | Utils.btoi(buf[8]);
		type = MessageType.values()[buf[9]];

		return msg_length;
	}
};

class IPAddress
{
	/* A set of routines to manipulate addresses in these forms:
	   dns.name
		dns.name:portno
		n.n.n.n
		n.n.n.n:portno
	*/
	
	static String check_add_port(String addr, int port)
	{
		String s;

		if(has_port(addr))
			s = addr;
		else
		{
			if(addr.contains(":"))
				s = addr + port;
			else
				s = addr + ":" + port;
		}
		return s;	
	}

	static boolean has_port(String addr)
	{
		int pos, len;
		boolean numeric = false;
		char c;
		
		pos = addr.indexOf(':');
		if(pos < 0)
			return false; // No colon in address
		len = addr.length();
		for(int i = pos + 1; i < len; i++)
		{
			c = addr.charAt(i);
			if(c >= '0' && c <= '9')
				numeric = true; // OK, seen at least one digit
			else if(c == ' ' || c == '\t' || c == '\n')
				continue; // Skip whitespace
			else
				Log.error("Address <" + addr + "> in invalid format");
		}
		return numeric;
	}

	// static int valid(String addr);
	// static int get_port(String addr);
	// static String get_machine(String addr);
};
