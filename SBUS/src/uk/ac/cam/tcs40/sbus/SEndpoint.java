package uk.ac.cam.tcs40.sbus;

public class SEndpoint {

	private String m_EndpointName;
	private String m_MessageHash;
	private String m_ResponseHash;
	
	private long m_EndpointPointer;

	SEndpoint(long ptr, String endpointName, String messageHash, String responseHash) {
		// Note - no modifier means package only access.
		// Can only create endpoint through addEndpoint().
		this.m_EndpointPointer = ptr;
		this.m_EndpointName = endpointName;
		this.m_MessageHash = messageHash;
		this.m_ResponseHash = responseHash;
	}
	
	long getPointer() {
		return this.m_EndpointPointer;
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
	 * @return The hash code for this endpoint message schema.
	 */
	public String getMessageHash() {
		return this.m_MessageHash;
	}
	
	/**
	 * 
	 * @return The hash code for this endpoint response schema.
	 */
	public String getResponseHash() {
		return this.m_ResponseHash;
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

	/**
	 * Map this endpoint to another endpoint.
	 * @param address The address of the endpoint to map to.
	 * @param endpoint The name of the endpoint to map to.
	 * @return The result of the map operation.
	 */
	public String map(String address, String endpoint) {
		return map(m_EndpointPointer, address, endpoint);
	}

	/**
	 * Unmap this endpoint from all other endpoints.
	 */
	public void unmap() {
		unmap(m_EndpointPointer);
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

	private native String map(long endpointPtr, String address, String endpoint);
	private native void unmap(long endpointPtr);
	
	private native long receive(long endpointPtr);
}
