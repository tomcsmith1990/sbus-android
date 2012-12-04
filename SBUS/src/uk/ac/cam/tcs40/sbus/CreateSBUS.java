package uk.ac.cam.tcs40.sbus;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import android.content.Context;
import android.os.Environment;
import android.util.Log;

public class CreateSBUS {
	
	private final File m_Directory;
	private final Context m_Context;
	
	public CreateSBUS(Context context) {
		super();
		this.m_Context = context;
		this.m_Directory = context.getFilesDir();
	}
	
	public String getDirectory() {
		return m_Directory.getAbsolutePath();
	}

	public void store(String filename) {
		if (!hasExternalStorageDownloadFile(filename))
	    	createExternalStorageDownloadFile(filename);
		/*
		boolean mExternalStorageAvailable = false;
		boolean mExternalStorageWriteable = false;
		String state = Environment.getExternalStorageState();

		if (Environment.MEDIA_MOUNTED.equals(state)) {
		    // We can read and write the media
		    mExternalStorageAvailable = mExternalStorageWriteable = true;
		    
		    //if (!hasExternalStorageDownloadFile(filename))
		    	//createExternalStorageDownloadFile(context, filename);
		    
		} else if (Environment.MEDIA_MOUNTED_READ_ONLY.equals(state)) {
		    // We can only read the media
		    mExternalStorageAvailable = true;
		    mExternalStorageWriteable = false;
		} else {
		    // Something else is wrong. It may be one of many other states, but all we need
		    //  to know is we can neither read nor write
		    mExternalStorageAvailable = mExternalStorageWriteable = false;
		}*/
	}
	
	private void createExternalStorageDownloadFile(String filename) {
	    // Create a path where we will the file.
		// Note that you should be careful about what you place here, 
		// since the user often manages these files.
	    File file = new File(m_Directory, filename);

	    try {
	        // Make sure directory exists.
	    	new File(file.getParent()).mkdirs();

	        // Very simple code to copy a picture from the application's
	        // resource into the external file.  Note that this code does
	        // no error checking, and assumes the picture is small (does not
	        // try to copy it in chunks).  Note that if external storage is
	        InputStream is = m_Context.getAssets().open(filename);
	        OutputStream os = new FileOutputStream(file);
	        
	        int bufferSize = 65536;
	        byte[] data = new byte[bufferSize];
	        int bytesRead = 0;
	        while ((bytesRead = is.read(data)) != -1) {
	        	os.write(data, 0, bytesRead);
	        }
	        
	        is.close();
	        os.close();
	        Log.i("set permission", "chmod 755 " + file.getAbsolutePath());
	        Runtime.getRuntime().exec("chmod 755 " + file.getAbsolutePath());
	        
	    } catch (IOException e) {
	        // Unable to create file, likely because external storage is
	        // not currently mounted.
	        Log.w("ExternalStorage", "Error writing " + file, e);
	    }
	}

	private void deleteExternalStorageDownloadFile(String filename) {
	    // Create a path where we will place the file and delete.
		// If external storage is not currently mounted this will fail.
	    File file = new File(m_Directory, filename);
	    file.delete();
	}

	private boolean hasExternalStorageDownloadFile(String filename) {
	    // Create a path where we will place the file and check if the file exists.
		// If external storage is not currently mounted this will think the
	    // file doesn't exist.
	    File file = new File(m_Directory, filename);
	    return file.exists();
	}
}
