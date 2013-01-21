package uk.ac.cam.tcs40.sbus;

public class Multiplex {

	private long m_MultiplexPtr;
	private SComponent m_Component;
	
	Multiplex(SComponent component) {
		this.m_MultiplexPtr = multiplex();
		this.m_Component = component;
	}
	
	public void add(SEndpoint endpoint) {
		add(m_MultiplexPtr, endpoint.getPointer());
	}
	
	public SEndpoint waitForMessage() {
		long ptr = waitForMessage(m_MultiplexPtr, m_Component.getPointer());
		return m_Component.getEndpointByPtr(ptr);
	}
	
	public void delete() {
		delete(this.m_MultiplexPtr);
	}
	
	private native long multiplex();
	private native void add(long multiplexPtr, long endpointPtr);
	private native long waitForMessage(long multiplexPtr, long componentPtr);
	private native void delete(long multiplexPtr);
}
