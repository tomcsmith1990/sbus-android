import java.util.*;
import java.util.concurrent.locks.*;
import java.io.*;

class Persistence
{
	ArrayList<String> cpts, procs;
	String sbus_dir;
	
	Persistence()
	{
		String filename;

		procs = null;

		// Read ~/.sbus/autorun
		filename = Log.get_sbus_dir() + "/autorun";
		cpts = Utils.readlinefile(filename, true);
		if(cpts == null)
		{
			Log.warning("Warning: Could not open " + filename +
					"; no persistent components list");
			cpts = new ArrayList<String>();
		}
	}
	
	void start(String cmd)
	{
		String[] args;
		Process p;
		Runtime runtime = Runtime.getRuntime();

		args = cmd.split("[\t ]+");
		try
		{
			p = runtime.exec(args);
		}
		catch(IOException e)
		{
			Log.warning("Warning: could not start persistent component -\n" +
					cmd);
		}
	}

	void start_all()
	{
		procs = running_procs();

		for(int i = 0; i < cpts.size(); i++)
		{
			checkquotes(cpts.get(i));
			System.out.println("Persistent cpt " + cpts.get(i) +
					(is_running(cpts.get(i)) ? " is running" : " is not running"));
			if(!is_running(cpts.get(i)))
				start(cpts.get(i));
		}
	}
	
	boolean is_persistent(String cmd)
	{
		for(int i = 0; i < cpts.size(); i++)
		{
			if(cpts.get(i).equals(cmd))
				return true;
		}
		return false;
	}

	private ArrayList<String> running_procs()
	{
		File dir;
		String[] names;
		BufferedReader in;
		String pathname, s;
		ArrayList<String> results = new ArrayList<String>();

		// Access /proc/<number>/cmdline
		dir = new File("/proc");
		if(!dir.isDirectory())
		{
			Log.warning("Cannot open /proc directory; " +
					"disabling running process check");
			return results;
		}
		names = dir.list();
		for(String filename: names)
		{
			if(!is_an_integer(filename))
				continue;
			pathname = "/proc/" + filename + "/cmdline";
			try
			{
				in = new BufferedReader(new FileReader(pathname));
				s = in.readLine(); // Only need to read one line
				if(s != null)
				{
					StringBuilder sb = new StringBuilder(s);
					for(int i = 0; i < sb.length(); i++)
					{
						if(sb.charAt(i) == '\0')
							sb.setCharAt(i, ' ');
					}
					results.add(sb.toString());
				}
				in.close();
			}
			catch(IOException e)
			{
				Log.warning("Warning: could not open " + pathname);
				continue;
			}
		}
		return results;
	}
	
	private boolean is_running(String proc)
	{
		for(int i = 0; i < procs.size(); i++)
		{
			if(procs.get(i).equals(proc))
				return true;
		}
		return false;
	}

	void hup(Endpoint ep, ArrayList<Image> live, Lock live_mutex,
			Endpoint terminate_ep)
	{
		Image img;
		String filename;
		ProtoMessage dummy;
		
		dummy = ep.rcv(); // Discard dummy

		System.out.println("Received hangup instruction: " +
				"re-checking persistent components list\n");

		// Refresh list of persistent components:
		filename = sbus_dir + "/autorun";
		cpts = Utils.readlinefile(filename, true);
		if(cpts == null)
		{
			Log.warning("Warning: Could not open " + filename +
					"; no persistent components list");
			cpts = new ArrayList<String>();
		}

		// Scan for any which are no longer persistent, and stop them:
		live_mutex.lock();
		for(int i = 0; i < live.size(); i++)
		{
			img = live.get(i);
			if(img.local && img.persistent)
			{
				if(!is_persistent(img.state.extract_txt("cmdline")))
				{
					// Stop it:
					System.out.println("Terminating previously persistent " +
							"component " + img.address);
					img.persistent = false;
					if(terminate_ep.map(img.address, "terminate") == null)
					{
						System.out.println("Failed to terminate " + img.address);
						continue;
					}
					terminate_ep.emit(null);
					terminate_ep.unmap();
				}
			}
		}
		live_mutex.unlock();

		// Start any which are not running:
		start_all();
	}
	
	// Utilities:
			
	private boolean is_an_integer(String s)
	{
		int pos, len = s.length();
		if(len == 0)
			return false;
		for(pos = 0; pos < len; pos++)
			if(s.charAt(pos) < '0' || s.charAt(pos) > '9')
				return false;
		if(pos > 20) // Integer getting too long for buffers
			return false;
		return true;
	}
	
	private void checkquotes(String cmd)
	{
		if(cmd.contains("\"") || cmd.contains("'"))
		{
			Log.error("Quotation marks not currently allowed in persistent " +
					"component command lines");
		}
	}
};
