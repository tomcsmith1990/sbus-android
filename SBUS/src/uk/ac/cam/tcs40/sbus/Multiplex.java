package uk.ac.cam.tcs40.sbus;

public class Multiplex {

	private long m_MultiplexPtr;
	private SComponent m_Component;
	
	Multiplex(SComponent component) {
		this.m_MultiplexPtr = multiplex();
		this.m_Component = component;
	}
	
	/**
	 * Add an endpoint to this Multiplex.
	 * @param endpoint The endpoint to add - must be part an endpoint on the component this multiplex belongs to.
	 */
	public void add(SEndpoint endpoint) {
		add(m_MultiplexPtr, endpoint.getPointer());
	}
	
	/**
	 * Wait until there is a message for one of the endpoints in this Multiplex.
	 * @return An endpoint which has a message waiting.
	 * @throws Exception If we have found a message for an endpoint not on this component.
	 */
	public SEndpoint waitForMessage() throws Exception {
		long ptr = waitForMessage(m_MultiplexPtr, m_Component.getPointer());
		SEndpoint endpoint = m_Component.getEndpointByPtr(ptr);
		if (endpoint == null)
			throw new Exception("Received a message for an endpoint which is not on this component");
		return endpoint;
	}
	
	/**
	 * Delete the native representation of this Multiplex.
	 */
	public void delete() {
		delete(this.m_MultiplexPtr);
	}
	
	private native long multiplex();
	private native void add(long multiplexPtr, long endpointPtr);
	private native long waitForMessage(long multiplexPtr, long componentPtr);
	private native void delete(long multiplexPtr);
}
