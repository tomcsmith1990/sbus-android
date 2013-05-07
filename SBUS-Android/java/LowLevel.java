// LowLevel.java - DMI - 9-5-2009

import java.io.*;
import java.util.*;
import java.net.*;
import java.nio.*;
import java.nio.channels.*;
import java.nio.charset.*;

class LowLevel
{
	static final int ACTIVE_SOCK_NORMAL = 0;
	static final int ACTIVE_SOCK_SILENT = 1;
	static final int ACTIVE_SOCK_NONBLOCK = 2;

	static final int DEFAULT_PORT = 9219;
	static final int DEFAULT_RDC_PORT = 50123;

	static class ServerSocketPort
	{
		ServerSocketChannel ssc;
		int port;
	}
	
	static ServerSocketPort passivesock(int port, boolean blocking)
	{
		// Returns ServerSocketChannel and actual port.
		// Requested port may be -1 for any.
		ServerSocketChannel ssc = null;
		ServerSocket ss = null;

		try
		{		
			ssc = ServerSocketChannel.open();
			ss = ssc.socket();
			ss.setReuseAddress(true);
			if(port == -1)
				ss.bind(null);
			else
				ss.bind(new java.net.InetSocketAddress(port));
			ssc.configureBlocking(blocking);
		}
		catch(Exception e)
		{
			Log.error("Can't create listening socket");
		}
		
		ServerSocketPort ssp = new ServerSocketPort();
		ssp.ssc = ssc;
		if(ss != null)
			ssp.port = ss.getLocalPort();
		else
			ssp.port = -1;
		return ssp;
	}

	static class SocketRemoteAddr
	{
		SocketChannel sc;
		String remote_address;
	}
		
	static SocketRemoteAddr acceptsock(ServerSocketChannel master_ssc)
	{
		// acceptsock used to return -1 in sock field on error
		// acceptsock now returns null on error
		SocketChannel dyn_sc;
		Socket sock;
		InetAddress peer;
		String remote_ip_address, remote_hostname;
		int remote_port;

		if(master_ssc == null)
			Log.error("DBG> master_ssc = null");				
		try
		{
			dyn_sc = master_ssc.accept();
			if(dyn_sc == null)
				Log.error("DBG> dyn_sc = null");
			dyn_sc.configureBlocking(false);
		}
		catch(IOException e)
		{
			Log.warning("Accept failed");
			return null;
		}
		sock = dyn_sc.socket();
		sock_nodelay(sock);
		peer = sock.getInetAddress();
		remote_ip_address = peer.getHostAddress();
		remote_hostname = peer.getCanonicalHostName();
		remote_port = sock.getPort();
		
		SocketRemoteAddr sra = new SocketRemoteAddr();
		sra.sc = dyn_sc;
		sra.remote_address = remote_ip_address + ":" + remote_port;
		return sra;
	}
	
	// All the activesock() variants return null on error:
	
	static SocketChannel activesock(String address, int flags)
	{
		// Address string may encode host and port
		int port = LowLevel.DEFAULT_PORT;
		String hostname = "localhost";
		int pos;
		
		if(address == null)
			return null;
		pos = address.indexOf(":");
		if(pos == 0)
		{
			// Port only:
			port = Integer.parseInt(address.substring(1));
		}
		else if(pos == address.length() - 1)
		{
			// Host only:
			hostname = address.substring(0, pos);
		}
		else if(pos == -1)
		{
			// Host only:
			hostname = address;
		}
		else
		{
			// Host and Port:
			hostname = address.substring(0, pos);
			port = Integer.parseInt(address.substring(pos + 1));
		}
		return do_connect(hostname, port, flags);
	}
	static SocketChannel activesock(String address)
	{
		// Address string may encode host and port
		return activesock(address, ACTIVE_SOCK_NORMAL);
	}
	
	static SocketChannel activesock(int port, int flags)
	{
		return do_connect("localhost", port, flags);
	}
	static SocketChannel activesock(int port)
	{
		return activesock(port, ACTIVE_SOCK_NORMAL);
	}
	
	static SocketChannel activesock(int port, String hostname,
			int flags)
	{
		return do_connect(hostname, port, flags);
	}
	static SocketChannel activesock(int port, String hostname)
	{
		return activesock(port, hostname, ACTIVE_SOCK_NORMAL);
	}

	private static SocketChannel do_connect(String remote_hostname,
			int port, int flags)
	{
		SocketChannel sc;		
		InetSocketAddress addr;
		
		try
		{
			addr = new InetSocketAddress(remote_hostname, port);
			sc = SocketChannel.open(addr);			
		}
		catch(IOException e)
		{
			if((flags & ACTIVE_SOCK_SILENT) == 0)
				Log.warning("Can't connect to " + remote_hostname);
			return null;
		}
		LowLevel.sock_nodelay(sc.socket());
		if((flags & ACTIVE_SOCK_NONBLOCK) != 0)
			sock_nonblock(sc);
		return sc;
	}
	
	static void sock_nodelay(Socket sock)
	{
		try
		{
			sock.setTcpNoDelay(true);
		}
		catch(IOException e)
		{
			Log.error("Can't set TcpNoDelay");
		}
	}

	static void sock_nonblock(SocketChannel sc)
	{
		try
		{
			sc.configureBlocking(false);
		}
		catch(IOException e)
		{
			Log.error("Can't set socket to nonblocking mode");
		}
	}
		
	static String get_local_ip()
	{
		InetAddress addr = null;
		String dotted;
		
		try
		{
			addr = InetAddress.getLocalHost();
		}
		catch(Exception e)
		{
			Log.error("Can't get local host IP");
		}
		dotted = addr.getHostAddress();
		return dotted;
	}
	
	static String get_local_address(Socket sock)
	{
		InetAddress addr;
		String dotted;
		int port;
		
		addr = sock.getLocalAddress();
		port = sock.getLocalPort();
		dotted = addr.getHostAddress();
		return dotted + ":" + port;
	}

	static boolean fixed_read(SocketChannel sc, byte[] buf, int nbytes)
	{
		int remain = nbytes;
		int amount;

		ByteBuffer bb = ByteBuffer.wrap(buf);		
		while(remain > 0)
		{
			try { amount = sc.read(bb); }
			catch(IOException e) { return false; }
			if(amount == 0)
				Utils.delay(1); // No data ready
			if(amount < 0)
				return false; // End of stream
			remain -= amount;
		}
		return true;
	}
	
	static boolean fixed_write(SocketChannel sc, byte[] buf, int nbytes)
	{
		int remain = nbytes;
		int amount;
		
		ByteBuffer bb = ByteBuffer.wrap(buf);
		while(remain > 0)
		{
			try { amount = sc.write(bb); }
			catch(IOException e) { return false; }
			if(amount == 0)
				Utils.delay(1); // No data ready
			remain -= amount;
		}
		return true;
	}
};

class Utils
{
	static void delay(int ms)
	{
		try { Thread.sleep(ms); }
		catch (InterruptedException e) {}
	}
	
	static String mixed_case(String s)
	{
		String ret;
		int len = s.length();
		
		if(len == 1)
			return s.toUpperCase();
		else if(len == 0)
			return s;
		ret = s.substring(0, 1).toUpperCase();
		ret += s.substring(1).toLowerCase();
		return ret;
	}
	
	static int btoi(byte b)
	{
		int x = b;
		if(x >= 0)
			return x;
		return 256 + x;
	}
	
	static boolean is_whitespace(String s)
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
	
	static ArrayList<String> readlinefile(String pathname,
			boolean filter_blanks)
	{
		ArrayList<String> v = new ArrayList<String>();
		BufferedReader in;
		String line;
		
		try
		{
			in = new BufferedReader(new FileReader(pathname));
			while((line = in.readLine()) != null)
			{
				if(filter_blanks && (is_whitespace(line) || line.charAt(0) == '#'))
					continue;
				v.add(line);
			}
			in.close();
		}
		catch(IOException e)
		{
			// "Cannot read file from <" + pathname + ">"
			return null;
		}
		
		return v;
	}
};
