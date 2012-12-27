package uk.ac.cam.tcs40.sbus;


public class SComponent {

	private long m_ComponentPointer;
	
	public SComponent(String componentName, String instanceName) {
		this.m_ComponentPointer = scomponent(componentName, instanceName);
	}
	
	public SEndpoint addEndpointSource(String name, String hash) {
		long ptr = addEndpointSource(m_ComponentPointer, name, hash);
		return new SEndpoint(ptr, name, hash);
	}
	
	public SEndpoint addEndpointSink(String name, String hash) {
		long ptr = addEndpointSink(m_ComponentPointer, name, hash);
		return new SEndpoint(ptr, name, hash);
	}
	
	public void addRDC(String rdcAddress) {
		addRDC(m_ComponentPointer, rdcAddress);
	}
	
	public void start(String cptFilename, int port, boolean useRDC) {
		start(m_ComponentPointer, cptFilename, port, useRDC);
	}
	
	public void setPermission(String componentName, String instanceName, boolean allow) {
		setPermission(m_ComponentPointer, componentName, instanceName, allow);
	}
	
	public void delete() {
		// Also deletes endpoint.
		delete(m_ComponentPointer);
	}
	
	private native long scomponent(String componentName, String instanceName);
	private native long addEndpointSource(long componentPtr, String endpointName, String endpointHash);
	private native long addEndpointSink(long componentPtr, String endpointName, String endpointHash);
	private native void addRDC(long componentPtr, String rdcAddress);
	private native void start(long componentPtr, String cptFilename, int port, boolean useRDC);
	private native void setPermission(long componentPtr, String componentName, String instanceName, boolean allow);
	private native void delete(long componentPtr);
	
	static {
		System.load("/data/data/uk.ac.cam.tcs40.sbus.sbus/lib/libsbusandroid.so");
    }	
}
