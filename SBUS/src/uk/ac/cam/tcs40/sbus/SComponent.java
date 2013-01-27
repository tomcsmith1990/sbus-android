package uk.ac.cam.tcs40.sbus;

import java.util.LinkedList;
import java.util.List;

public class SComponent {

	public enum EndpointType { EndpointSink, EndpointSource, EndpointClient, EndpointServer };

	private long m_ComponentPointer;
	private SEndpoint m_RDCUpdateNotifications;
	private List<SEndpoint> m_Endpoints = new LinkedList<SEndpoint>();

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
		SEndpoint endpoint;
		
		switch (type) {
		case EndpointSink:
			ptr = addEndpointSink(m_ComponentPointer, name, messageHash, responseHash);
			break;
			
		case EndpointSource:
			ptr = addEndpointSource(m_ComponentPointer, name, messageHash, responseHash);
			break;
			
		case EndpointClient:
			ptr = addEndpointClient(m_ComponentPointer, name, messageHash, responseHash);
			break;
			
		default:
			return null;
		}
		endpoint = new SEndpoint(ptr, name, messageHash, responseHash);
		m_Endpoints.add(endpoint);
		return endpoint;
	}

	/**
	 * Add the address to the list of RDCs for when this component starts.
	 * Try to register with the RDC is it is already started.
	 * @param rdcAddress The address (IP:port) of the RDC.
	 */
	public void addRDC(String rdcAddress) {
		addRDC(m_ComponentPointer, rdcAddress);
	}
	
	/**
	 * Remove the address from the list of RDCs for when this component starts.
	 * Try to deregister from the RDC is it is already started.
	 * @param rdcAddress The address (IP:port) of the RDC.
	 */
	public void removeRDC(String rdcAddress) {
		removeRDC(m_ComponentPointer, rdcAddress);
	}
	
	/**
	 * Whether or not the rdc_update notification endpoint should be sent notifications.
	 * @param notify
	 */
	public void setRDCUpdateNotify(boolean notify) {
		setRDCUpdateNotify(m_ComponentPointer, notify);
	}
	
	/**
	 * Whether or not the component should automatically connect to new RDCs.
	 * @param autoconnect
	 */
	public void setRDCUpdateAutoconnect(boolean autoconnect) {
		setRDCUpdateAutoconnect(m_ComponentPointer, autoconnect);
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
	 * 
	 * @return An endpoint which will receive messages when an RDC is added/removed.
	 */
	public SEndpoint RDCUpdateNotificationsEndpoint() {
		if (m_RDCUpdateNotifications == null) {
			long ptr = RDCUpdateNotificationsEndpoint(m_ComponentPointer);
			m_RDCUpdateNotifications = new SEndpoint(ptr, "rdc_update", "13ACF49714C5", null);
			m_Endpoints.add(m_RDCUpdateNotifications);
		}
		return m_RDCUpdateNotifications;
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
	 * 
	 * @return The Multiplex for this component - for waiting for messages to arrive.
	 */
	public Multiplex getMultiplex() {
		return new Multiplex(this);
	}
	
	SEndpoint getEndpointByPtr(long ptr) {
		for (SEndpoint endpoint : m_Endpoints) {
			if (endpoint.getPointer() == ptr)
				return endpoint;
		}
		return null;
	}
	
	long getPointer() {
		return this.m_ComponentPointer;
	}

	/**
	 * Delete this component (and its endpoints).
	 * Must be called when finished, deletes the native representation.
	 */
	public void delete() {
		delete(m_ComponentPointer);
	}

	private native long scomponent(String componentName, String instanceName);
	
	private native long addEndpointSource(long componentPtr, String endpointName, String messageHash, String responseHash);
	private native long addEndpointSink(long componentPtr, String endpointName, String messageHash, String responseHash);
	private native long addEndpointClient(long componentPtr, String endpointName, String messageHash, String responseHash);
	
	private native void addRDC(long componentPtr, String rdcAddress);
	private native void removeRDC(long componentPtr, String rdcAddress);
	private native void setRDCUpdateAutoconnect(long componentPtr, boolean autoconnect);
	private native void setRDCUpdateNotify(long componentPtr, boolean notify);
	
	private native void start(long componentPtr, String cptFilename, int port, boolean useRDC);

	// May only call after start().
	private native String declareSchema(long componentPtr, String schema);
	private native long RDCUpdateNotificationsEndpoint(long componentPtr);
	private native void setPermission(long componentPtr, String componentName, String instanceName, boolean allow);
	
	private native void delete(long componentPtr);

	static {
		System.load("/data/data/uk.ac.cam.tcs40.sbus.sbus/lib/libsbusandroid.so");
	}	
}
