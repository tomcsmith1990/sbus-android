package uk.ac.cam.tcs40.sbus;

public class SComponent {

	public SComponent(String componentName, String instanceName) {
		scomponent(componentName, instanceName);
	}
	
	public void addEndpoint(SEndpoint sendpoint) {
		String name = sendpoint.getEndpointName();
		String hash = sendpoint.getEndpointHash();
		long ptr = addEndpoint(name, hash);
		sendpoint.setPointer(ptr);
	}
	
	public native void scomponent(String componentName, String instanceName);
	public native void addRDC(String rdcAddress);
	public native void start(String cptFilename, int port, boolean useRDC);
	public native void setPermission(String componentName, String instanceName, boolean allow);
	public native void delete();
	
	private native long addEndpoint(String endpointName, String endpointHash);
	
	static {
		System.load("/data/data/uk.ac.cam.tcs40.sbus.sbus/lib/libsbusandroid.so");
    }	
}
