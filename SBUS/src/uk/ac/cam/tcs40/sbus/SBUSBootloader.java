package uk.ac.cam.tcs40.sbus;

import android.content.Context;

public class SBUSBootloader extends FileBootloader {

	private final String[] m_Files = 
		{
			"idl/broker.cpt",
			"idl/builtin.cpt",
			"idl/carpark.cpt",
			"idl/cpt_metadata.idl",
			"idl/cpt_status.idl",
			"idl/demo.cpt",
			"idl/map_constraints.idl",
			"idl/rdc_default.priv",
			"idl/rdc.cpt",
			"idl/rdcacl.cpt",
			"idl/sbus.cpt",
			"idl/slowcar.cpt",
			"idl/speek.cpt",
			"idl/spersist.cpt",
			"idl/spoke.cpt",
			"idl/trafficgen.cpt",
			"idl/universalsink.cpt",
			"idl/universalsource.cpt",
			"sbuswrapper"
		};

	public SBUSBootloader(Context context) {
		super(context);

		// Copy the idl files and sbuswrapper if they don't exist.
		// Also sets correct permissions on them.
		for (String filename : m_Files) {
			store(filename);
			setPermissions(filename);
		}
		// Set permissions on /idl directory.
		setPermissions(getApplicationDirectory() + "/idl");
	}

}
