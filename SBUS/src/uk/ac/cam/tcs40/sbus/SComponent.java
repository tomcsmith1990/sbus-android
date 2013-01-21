package uk.ac.cam.tcs40.sbus;


public class SComponent {

	public enum EndpointType { EndpointSink, EndpointSource, EndpointClient, EndpointServer };

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
	 * Add an endpoint to this component.
	 * @param name The name of the endpoint.
	 * @param type The type of endpoint.
	 * @param messageHash The hash of the endpoint message schema.
	 * @return An SEndpoint representing this endpoint.
	 */
	public SEndpoint addEndpoint(String name, EndpointType type, String messageHash) {
		return addEndpoint(name, type, messageHash, null);
	}

	/**
	 * Add an endpoint to this component.
	 * @param name The name of the endpoint.
	 * @param type The type of endpoint.
	 * @param messageHash The hash of the endpoint message schema.
	 * @param responseHash The hash of the the endpoint response schema.
	 * @return An SEndpoint representing this endpoint.
	 */
	public SEndpoint addEndpoint(String name, EndpointType type, String messageHash, String responseHash) {
		long ptr;
		
		switch (type) {
		case EndpointSink:
			ptr = addEndpointSink(m_ComponentPointer, name, messageHash, responseHash);
			return new SEndpoint(ptr, name, messageHash, responseHash);

		case EndpointSource:
			ptr = addEndpointSource(m_ComponentPointer, name, messageHash, responseHash);
			return new SEndpoint(ptr, name, messageHash, responseHash);

		case EndpointClient:
			ptr = addEndpointClient(m_ComponentPointer, name, messageHash, responseHash);
			return new SEndpoint(ptr, name, messageHash, responseHash);
			
		default:
			return null;
		}
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
		// Also deletes endpoints.
		delete(m_ComponentPointer);
	}

	private native long scomponent(String componentName, String instanceName);
	private native long addEndpointSource(long componentPtr, String endpointName, String messageHash, String responseHash);
	private native long addEndpointSink(long componentPtr, String endpointName, String messageHash, String responseHash);
	private native long addEndpointClient(long componentPtr, String endpointName, String messageHash, String responseHash);
	private native void addRDC(long componentPtr, String rdcAddress);
	private native void start(long componentPtr, String cptFilename, int port, boolean useRDC);
	
	// May only call after start().
	private native void setPermission(long componentPtr, String componentName, String instanceName, boolean allow);
	private native String declareSchema(long componentPtr, String schema);
	private native void delete(long componentPtr);

	static {
		System.load("/data/data/uk.ac.cam.tcs40.sbus.sbus/lib/libsbusandroid.so");
	}	
}
