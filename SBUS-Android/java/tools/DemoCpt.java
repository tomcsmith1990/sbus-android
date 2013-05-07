import java.util.*;
import java.nio.*;
import java.nio.channels.*;

class DemoCpt
{
	static int log_use_count, stream_use_count;
	static String instance_name;

	static final int count_ms = 1000;

	static void usage()
	{
		System.out.println("Usage: java DemoCpt [instance-name]");
		System.exit(0);
	}

	static void parse_args(String[] args)
	{
		instance_name = null;

		if(args.length > 1)
			usage();
		if(args.length == 1)
		{
			if(args[0].charAt(0) == '-')
				usage();
			instance_name = args[0];
		}
	}

	public static void main(String[] args)
	{
		String cpt_filename = "demo.cpt";
		Component com;
		Endpoint ep, news_ep, log_ep, counter_ep;
		SocketChannel sc = null;
		Selector selector = null;
		long ms, ms_last, ms_now;
		int count = 0;

		parse_args(args);
		Component.set_log_level(Log.LogAll, Log.LogAll); // For debugging
		com = new Component("demo", instance_name);
		log_ep = com.add_endpoint("log", EndpointType.Sink, "B732FBF25198");
		com.add_endpoint("usage", EndpointType.Server, "000000000000",
				"C50A50E9216F");
		com.add_endpoint("submit", EndpointType.Sink, "D39F55D56AF2");
		news_ep = com.add_endpoint("news", EndpointType.Source, "D39F55D56AF2");
		com.add_endpoint("delay", EndpointType.Server, "2632D5B0A26B",
				"000000000000");
		counter_ep = com.add_endpoint("counter", EndpointType.Source,
				"85EC3009D66C");
		com.add_endpoint("viewer", EndpointType.Sink, "85EC3009D66C");

		log_use_count = stream_use_count = 0;
		com.start(cpt_filename);
		log_ep.subscribe(null, "message ~ 'TIME rolls'", null);

		// Get SocketChannels:
		try
		{
			selector = Selector.open();
			for(int i = 0; i < com.count_endpoints(); i++)
			{
				sc = com.get_endpoint(i).sc;
				sc.register(selector, SelectionKey.OP_READ);
			}
		}
		catch(Exception e)
		{
			System.out.println("Cannot register with Selector.");
			System.exit(0);
		}

		// Selector loop:
		System.out.println("Entering democpt's event loop");
		ms_last = System.currentTimeMillis();
		while(true)
		{
			ms_now = System.currentTimeMillis();
			ms = ms_now - ms_last;
			ms = count_ms - ms;
			if(ms <= 0)
			{
				do_counter(counter_ep, count++);
				ms_last = System.currentTimeMillis();
				continue;
			}
			
			sc = null;
			try
			{ selector.select(ms); }
			catch(Exception e) {}
			Iterator it = selector.selectedKeys().iterator();
			while(it.hasNext())
			{
				SelectionKey selKey = (SelectionKey)it.next();

				// Remove it from the list to indicate that it is being processed
				it.remove();
				
				if(selKey.isValid() && selKey.isReadable())
					sc = (SocketChannel)selKey.channel();
				
				ep = com.sc_to_endpoint(sc);
				if(ep == null)
					continue;
				if(ep.name.equals("log")) do_log(com, ep);
				else if(ep.name.equals("usage")) do_usage(ep);
				else if(ep.name.equals("submit")) do_submit(ep, news_ep);
				else if(ep.name.equals("news")) do_news(ep);
				else if(ep.name.equals("delay")) do_delay(ep);
				else if(ep.name.equals("viewer")) do_view(ep);
			}			
		}
	}

	static void do_view(Endpoint ep)
	{
		ProtoMessage msg;
		String s;
		int n;

		msg = ep.rcv();
		s = msg.tree.extract_txt("word");
		n = msg.tree.extract_int("n");
		System.out.println(s + " " + n);
	}

	static void do_counter(Endpoint ep, int n)
	{
		Node node;
		String s;

		if(instance_name == null)
			s = "default";
		else
			s = instance_name;

		node = Node.pack(Node.pack(s, "word"), Node.pack(n, "n"), "counter");
		ep.emit(node);
	}

	static void do_log(Component com, Endpoint ep)
	{
		ProtoMessage msg;
		String s;

		msg = ep.rcv();
		s = msg.tree.extract_txt();
		if(s.equals("status?"))
		{
			Node node;

			node = com.get_status();
			node.dump();
		}
		else
		{
			System.out.println("Logging message: " + s);
		}
		log_use_count++;
	}

	static void do_usage(Endpoint ep)
	{
		ProtoMessage query;
		Node result;

		System.out.println("Component usage() implementation called");
		query = ep.rcv();
		result = Node.pack(Node.pack(log_use_count), Node.pack(stream_use_count));
		ep.reply(query, result);
	}

	static void do_submit(Endpoint ep, Endpoint news_ep)
	{
		ProtoMessage msg;

		msg = ep.rcv();
		news_ep.emit(msg.tree);
		stream_use_count++;
	}

	static void do_news(Endpoint ep)
	{
		System.out.println("Impossible: source endpoints never return " +
				"information");
		System.exit(0);
	}

	static void do_delay(Endpoint ep)
	{
		ProtoMessage msg;
		Node empty = Node.pack_empty();
		int secs, micros = 0;

		msg = ep.rcv();
		secs = msg.tree.extract_int("seconds");
		if(msg.tree.exists("micros"))
			micros = msg.tree.extract_int("micros");

		// Currently this makes the whole component go to sleep; in reality
		// we would return from do_delay and set a timer for the reply.
		
		delay(secs * 1000 + micros / 1000);
		ep.reply(msg, empty);
	}

	static void delay(int ms)
	{
		try { Thread.sleep(ms); }
		catch (InterruptedException e) {}
	}
};
