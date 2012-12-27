package uk.ac.cam.tcs40.sbus;

public class SEndpoint {

	private String m_EndpointName;
	private String m_EndpointHash;
	
	private long m_EndpointPointer;

	public SEndpoint(long ptr, String endpointName, String endpointHash) {
		this.m_EndpointPointer = ptr;
		this.m_EndpointName = endpointName;
		this.m_EndpointHash = endpointHash;	
	}

	public String getEndpointName() {
		return this.m_EndpointName;
	}

	public String getEndpointHash() {
		return this.m_EndpointHash;
	}

	public SNode createMessage(String messageRoot) {
		long ptr = createMessage(m_EndpointPointer, messageRoot);
		return new SNode(ptr);
	}

	public String emit(SNode node) {
		String s = emit(m_EndpointPointer, node.getPointer());
		node.delete();
		return s;
	}

	public String endpointMap(String address) {
		return endpointMap(m_EndpointPointer, address);
	}

	public void endpointUnmap() {
		endpointUnmap(m_EndpointPointer);
	}
	
	public SMessage receive() {
		long ptr = receive(m_EndpointPointer);
		return new SMessage(ptr);
	}

	private native long createMessage(long endpointPtr, String messageType);
	private native String emit(long endpointPtr, long messagePtr);

	private native String endpointMap(long endpointPtr, String address);
	private native void endpointUnmap(long endpointPtr);
	
	private native long receive(long endpointPtr);
}
