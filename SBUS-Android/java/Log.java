import java.io.*;

class Log
{
	static final int LogNothing   = 0;
	static final int LogErrors    = 1 << 0;
	static final int LogWarnings  = 1 << 1;
	static final int LogMessages  = 1 << 2;
	static final int LogDebugging = 1 << 3;
	static final int LogDefault =
			LogErrors | LogWarnings | LogMessages | LogDebugging;
	static final int EchoDefault = LogErrors | LogWarnings | LogMessages;
	static final int LogAll =
			LogErrors | LogWarnings | LogMessages | LogDebugging;

	static int log_level = LogDefault;
	static int echo_level = EchoDefault;

	static PrintStream fp_log = null;

	static void init_levels()
	{
		// Read SBUS_LOG_LEVEL and SBUS_ECHO_LEVEL; set log_level and echo_level
		String s;

		s = System.getenv("SBUS_LOG_LEVEL");
		if(s != null)
			log_level = Integer.valueOf(s);
		s = System.getenv("SBUS_ECHO_LEVEL");
		if(s != null)
			echo_level = Integer.valueOf(s);
	}

	static void error(String msg)
	{
		if((log_level & LogErrors) != 0 && fp_log != null)
		{
			fp_log.println(msg);
			fp_log.flush();
		}
		if((echo_level & LogErrors) != 0)
			System.err.println(msg);
		System.exit(-1);
	}

	static void log(String msg)
	{
		if((log_level & LogMessages) != 0 && fp_log != null)
		{
			fp_log.println(msg);
			fp_log.flush();
		}
		if((echo_level & LogMessages) != 0)
			System.err.println(msg);
	}

	static void warning(String msg)
	{
		if((log_level & LogWarnings) != 0 && fp_log != null)
		{
			fp_log.println(msg);
			fp_log.flush();
		}
		if((echo_level & LogWarnings) != 0)
			System.err.println(msg);
	}

	static void debug(String msg)
	{
		if((log_level & LogDebugging) != 0 && fp_log != null)
		{
			fp_log.println(msg);
			fp_log.flush();
		}
		if((echo_level & LogDebugging) != 0)
			System.err.println(msg);
	}
	
	static void sassert(int t, String msg)
	{
		if(t != 0)
			return;
		error("Assertion failed: " + msg);
	}
	
	static void init_logfile(String cpt_name, String instance_name,
			boolean wrapper)
	{
		// Set fp_log:
		String s, dir;
		char type;
		File f;
		String slash = System.getProperty("file.separator");

		dir = get_sbus_dir();

		s = dir + slash + "log";
		f = new File(s);
		if(f.exists() == false)
		{
			warning("Log directory " + s + " doesn't exist; creating it");
			if(f.mkdir() == false)
				error("Failed to create log directory");
		}

		if(wrapper)
			type = 'W';
		else
			type = 'L';
		if(instance_name == null || cpt_name.equals(instance_name))
			s = s + slash + cpt_name + "-" + type + ".log";
		else
			s = s + slash + cpt_name + "-" + instance_name + "-" + type + ".log";

		try { fp_log = new PrintStream(s); }
		catch(IOException e)
		{ error("Can't open log file " + s); }

		init_levels();
	}

	static String get_sbus_dir()
	{
		String dir;
		File f;

		dir = System.getenv("SBUS_DIR");
		if(dir == null)
		{
			// Try ~/.sbus instead:
			String homedir = System.getProperty("user.home");
			String slash = System.getProperty("file.separator");
			dir = homedir + slash + ".sbus";			
		}
		f = new File(dir);
		if(f.exists() == false)
		{
			warning("SBUS directory " + dir + " doesn't exist; creating it");
			if(f.mkdir() == false)
				error("Failed to create SBUS directory");
		}
		if(f.canRead() == false || f.isDirectory() == false)
			error("Cannot access SBUS directory <" + dir + ">");
		return dir;
	}
	
	static void wrapper_failed()
	{
		error("SBUS Component shutting down because wrapper terminated");
	}
};

class SBUSException extends Exception {};
class MatchException extends SBUSException {};
class DisconnectException extends SBUSException {};
class IncomparableTypesException extends SBUSException {};

class LabelledException extends SBUSException
{
	String msg;
	
	LabelledException(String s)
	{ msg = s; }
};

class ProtocolException extends LabelledException
{ ProtocolException(String s) { super(s); }};
class SubscriptionException extends LabelledException
{ SubscriptionException(String s) { super(s); }};
class ValidityException extends LabelledException
{ ValidityException(String s) { super(s); }};
class ImportException extends LabelledException
{ ImportException(String s) { super(s); }};

class SchemaException extends SBUSException
{
	String msg;
	int line;
	
	SchemaException(String s, int l)
	{
		msg = s;
		line = l;
	}
};
