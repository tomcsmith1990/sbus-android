package uk.ac.cam.tcs40.sbus;

public class Multiplex {

	private long m_MultiplexPtr;
	
	public Multiplex() {
		this.m_MultiplexPtr = multiplex();
	}
	
	public void add(SEndpoint endpoint) {
		add(m_MultiplexPtr, endpoint.getPointer());
	}
	
	public void delete() {
		delete(this.m_MultiplexPtr);
	}
	
	private native long multiplex();
	private native void add(long multiplexPtr, long endpointPtr);
	private native void delete(long multiplexPtr);
}
