package uk.ac.cam.tcs40.sbus;

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
		
	public native String emit();
	
	public native void delete();
	
	public native String endpointMap(String address);
	public native void endpointUnmap();
	
	static {
        //System.loadLibrary("sbusandroid");
		System.load("/data/data/uk.ac.cam.tcs40.sbus.sbus/lib/libsbusandroid.so");
    }	
}
