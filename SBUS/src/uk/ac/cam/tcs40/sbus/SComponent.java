package uk.ac.cam.tcs40.sbus;

public class SComponent {

	private long m_ComponentPointer;
	
	public SComponent(String componentName, String instanceName) {
		this.m_ComponentPointer = scomponent(componentName, instanceName);
	}
	
	public SEndpoint addEndpoint(String name, String hash) {
		return addEndpoint(name, true, hash);
	}
	
	public SEndpoint addEndpoint(String name, boolean source, String hash) {
		long ptr = addEndpoint(m_ComponentPointer, name, source, hash);
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
	private native long addEndpoint(long componentPtr, String endpointName, boolean source, String endpointHash);
	private native void addRDC(long componentPtr, String rdcAddress);
	private native void start(long componentPtr, String cptFilename, int port, boolean useRDC);
	private native void setPermission(long componentPtr, String componentName, String instanceName, boolean allow);
	private native void delete(long componentPtr);
	
	static {
		System.load("/data/data/uk.ac.cam.tcs40.sbus.sbus/lib/libsbusandroid.so");
    }	
}
