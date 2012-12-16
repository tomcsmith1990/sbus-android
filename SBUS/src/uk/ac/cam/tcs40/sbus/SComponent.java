package uk.ac.cam.tcs40.sbus;

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Date;

public class SComponent {

	public SComponent(String componentName, String instanceName) {
		scomponent(componentName, instanceName);
	}
	
	public native void scomponent(String componentName, String instanceName);
	public native void addEndpoint(String endpointName, String endpointHash);
	public native void addRDC(String rdcAddress);
	public native void start(String cptFilename, int port, boolean useRDC);
	public native void setPermission(String componentName, String instanceName, boolean allow);
	
	public native void createMessage(String messageType);
	
	public void packInt(int n) { packInt(n, null); }
	public native void packInt(int n, String name);
	
	public void packString(String s) { packString(s, null); }
	public native void packString(String s, String name);
	
	public void packTime(Date d) { packTime(d, null); }
	public void packTime(Date d, String name) {
		DateFormat format = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss+00");
		packClock(format.format(d), name);
	}
	public native void packClock(String date, String name);
		
	public native String emit();
	
	public native void delete();
	
	public native String endpointMap(String address);
	public native void endpointUnmap();
	
	static {
        //System.loadLibrary("sbusandroid");
		System.load("/data/data/uk.ac.cam.tcs40.sbus.sbus/lib/libsbusandroid.so");
    }	
}
