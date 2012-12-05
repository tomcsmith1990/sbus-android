package uk.ac.cam.tcs40.sbus;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import android.content.Context;
import android.util.Log;

public class FileBootloader {

	private final File m_Directory;
	private final Context m_Context;


	public FileBootloader(Context context) {
		super();
		this.m_Context = context;
		this.m_Directory = context.getFilesDir();
	}

	protected String getApplicationDirectory() {
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

		} catch (IOException e) {
			Log.w("createFile", "Error writing " + filename, e);
		}
	}

	/***
	 * Set the file permissions to rwxr-xr-x so everyone can read and execute.
	 * @param file File to set permissions on.
	 */
	protected void setPermissions(String filename) {
		try {
			Runtime.getRuntime().exec("chmod 755 " + filename);
		} catch (IOException e) {
			Log.w("setPermissions", "Error setting permissions to 755 on " + filename, e);
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
