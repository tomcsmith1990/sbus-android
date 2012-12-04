package uk.ac.cam.tcs40.sbus;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import android.content.Context;
import android.util.Log;

public class SBUSBootloader {

	private final File m_Directory;
	private final Context m_Context;

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
		super();
		this.m_Context = context;
		this.m_Directory = context.getFilesDir();
		
		// Copy the idl files and sbuswrapper if they don't exist.
		// Also sets correct permissions on them.
		for (String filename : m_Files) {
			store(filename);
		}
		// Set permissions on /idl directory.
		setPermissions(new File(getApplicationDirectory(), "/idl"));
	}

	private String getApplicationDirectory() {
		return m_Directory.getAbsolutePath();
	}

	public void store(String filename) {
		if (!fileExists(filename))
			createFile(filename);
	}

	/***
	 * Copy filename from the assets folder to the internal storage of this application.
	 * @param filename File to copy from assets folder.
	 */
	private void createFile(String filename) {
		File file = new File(m_Directory, filename);

		try {
			// Make sure directory exists.
			m_Directory.mkdirs();

			// Open an InputStream to the file.
			InputStream is = m_Context.getAssets().open(filename);
			// Open an OutputStream for the file.
			OutputStream os = new FileOutputStream(file);

			// Copy the file.
			int bufferSize = 65536;
			byte[] data = new byte[bufferSize];
			int bytesRead = 0;
			while ((bytesRead = is.read(data)) != -1) {
				os.write(data, 0, bytesRead);
			}

			// Close the file streams.
			is.close();
			os.close();

			setPermissions(file);

		} catch (IOException e) {
			Log.w("createFile", "Error writing " + filename, e);
		}
	}

	/***
	 * Set the file permissions to rwxr-xr-x so everyone can read and execute.
	 * @param file File to set permissions on.
	 */
	private void setPermissions(File file) {
		try {
			Runtime.getRuntime().exec("chmod 755 " + file.getAbsolutePath());
		} catch (IOException e) {
			Log.w("setPermissions", "Error setting permissions to 755 on " + file.getName(), e);
		}
	}

	/***
	 * Check if the file already exists in the application internal storage.
	 * @param filename File to check.
	 * @return true if there is a file with this filename in the application's internal storage.
	 */
	private boolean fileExists(String filename) {
		File file = new File(m_Directory, filename);
		return file.exists();
	}
}
