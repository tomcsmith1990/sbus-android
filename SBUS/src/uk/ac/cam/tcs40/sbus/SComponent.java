package uk.ac.cam.tcs40.sbus;


public class SComponent {

	private long m_ComponentPointer;
	
	/**
	 * 
	 * @param componentName The name of this component.
	 * @param instanceName The instance name of this component.
	 */
	public SComponent(String componentName, String instanceName) {
		this.m_ComponentPointer = scomponent(componentName, instanceName);
	}
	
	/**
	 * Add an endpoint source to this component.
	 * @param name The name of the endpoint.
	 * @param hash The hash of the endpoint schema.
	 * @return An SEndpoint representing this endpoint.
	 */
	public SEndpoint addEndpointSource(String name, String hash) {
		long ptr = addEndpointSource(m_ComponentPointer, name, hash);
		return new SEndpoint(ptr, name, hash);
	}
	
	/**
	 * Add an endpoint sink to this component.
	 * @param name The name of the endpoint.
	 * @param hash The hash of the endpoint schema.
	 * @return An SEndpoint representing this endpoint.
	 */
	public SEndpoint addEndpointSink(String name, String hash) {
		long ptr = addEndpointSink(m_ComponentPointer, name, hash);
		return new SEndpoint(ptr, name, hash);
	}
	
	/**
	 * Add the address to the list of RDCs for when this component starts.
	 * @param rdcAddress The address (IP:port) of the RDC.
	 */
	public void addRDC(String rdcAddress) {
		addRDC(m_ComponentPointer, rdcAddress);
	}
	
	/**
	 * Start the component.
	 * @param cptFilename The component schema file location.
	 * @param port The port to start on, or -1 for any port.
	 * @param useRDC Connect to the RDC on startup.
	 */
	public void start(String cptFilename, int port, boolean useRDC) {
		start(m_ComponentPointer, cptFilename, port, useRDC);
	}
	
	/**
	 * Set permissions for this other components connecting.
	 * @param componentName The peer component name.
	 * @param instanceName The peer instance name.
	 * @param allow Allow access to this component.
	 */
	public void setPermission(String componentName, String instanceName, boolean allow) {
		setPermission(m_ComponentPointer, componentName, instanceName, allow);
	}
	
	/**
	 * Declare a new schema on this component.
	 * @param schema The schema being declared.
	 * @return The hash of the schema.
	 */
	public String declareSchema(String schema) {
		return declareSchema(m_ComponentPointer, schema);
	}
	
	/**
	 * Delete this component (and its endpoints).
	 * Must be called when finished, deletes the native representation.
	 */
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
	private native String declareSchema(long componentPtr, String schema);
	private native void delete(long componentPtr);
	
	static {
		System.load("/data/data/uk.ac.cam.tcs40.sbus.sbus/lib/libsbusandroid.so");
    }	
}
