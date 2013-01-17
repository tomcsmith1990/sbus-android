package uk.ac.cam.tcs40.sbus.mapper;

import uk.ac.cam.tcs40.sbus.FileBootloader;
import android.content.Context;

public class SBUSBootloader extends FileBootloader {

	private final String[] m_Files = 
		{
			"idl/cpt_metadata.idl",
			"idl/cpt_status.idl",
			"idl/map_constraints.idl",
			"sbuswrapper"
		};

	public SBUSBootloader(Context context) {
		super(context);

		// Create and set permissions on /idl directory.
		mkdir(getApplicationDirectory() + "/idl");
		setPermissions("idl", 755);

		mkdir(getApplicationDirectory() + "/.sbus/log");
		setPermissions(".sbus", 777);
		setPermissions(".sbus/log", 777);

		// Copy the idl files and sbuswrapper if they don't exist.
		// Also sets correct permissions on them.
		for (String filename : m_Files) {
			store(filename);
			setPermissions(filename, 755);
		}
	}

}
