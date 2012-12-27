package uk.ac.cam.tcs40.sbus;

public class SEndpoint {

	private String m_EndpointName;
	private String m_EndpointHash;
	
	private long m_EndpointPointer;

	SEndpoint(long ptr, String endpointName, String endpointHash) {
		// Note - no modifier means package only access.
		// Can only create endpoint through addEndpoint().
		this.m_EndpointPointer = ptr;
		this.m_EndpointName = endpointName;
		this.m_EndpointHash = endpointHash;	
	}

	/**
	 * 
	 * @return The name of this endpoint.
	 */
	public String getEndpointName() {
		return this.m_EndpointName;
	}

	/**
	 * 
	 * @return The hash code for this endpoint schema.
	 */
	public String getEndpointHash() {
		return this.m_EndpointHash;
	}

	/**
	 * Create a message to be emitted on this endpoint.
	 * @param messageRoot The root of the message schema.
	 * @return An SNode which the message can be built on.
	 */
	public SNode createMessage(String messageRoot) {
		long ptr = createMessage(m_EndpointPointer, messageRoot);
		return new SNode(ptr);
	}

	/**
	 * Emit the message from this endpoint.
	 * Delete the native representation of the message.
	 * @param node The message to be emitted.
	 * @return XML representation of the message.
	 */
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
	
	/**
	 * Blocks until a message is received.
	 * @return The message received.
	 */
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
