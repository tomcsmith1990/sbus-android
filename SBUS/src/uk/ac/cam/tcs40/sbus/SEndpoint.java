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

	public void setAutomapPolicy(String address, String endpoint) {
		setAutomapPolicy(new Policy(address, endpoint));
	}

	public void setAutomapPolicy(Policy policy) {
		setAutomapPolicy(m_EndpointPointer, 
				policy.getRemoteAddress(), 
				policy.getRemoteEndpoint(), 
				policy.getSensor().ordinal(),
				policy.getCondition().ordinal(), 
				policy.getValue());
	}

	/**
	 * Blocks until a message is received.
	 * @return The message received.
	 */
	public SMessage receive() {
		long ptr = receive(m_EndpointPointer);
		return new SMessage(ptr);
	}

	/**
	 * Reply to an RPC.
	 * @param query The original query.
	 * @param reply The reply to send.
	 */
	public void reply(SMessage query, SNode reply) {
		reply(m_EndpointPointer, query.getPointer(), reply.getPointer());
	}

	/**
	 * Perform an RPC and get message back.
	 * @param query A query to send - can be null.
	 * @return SMessage containing return value of RPC.
	 */
	public SMessage rpc(SNode query) {
		long ptr;
		if (query == null)
			ptr = rpc(m_EndpointPointer, 0);
		else
			ptr = rpc(m_EndpointPointer, query.getPointer());

		if (ptr == 0)
			return null;
		else
			return new SMessage(ptr);
	}

	private native long createMessage(long endpointPtr, String messageType);
	private native String emit(long endpointPtr, long messagePtr);

	private native String map(long endpointPtr, String address, String endpoint);
	private native void unmap(long endpointPtr);
	private native void setAutomapPolicy(long endpointPtr, String address, String endpoint, int sensor, int condition, int value);


	private native long receive(long endpointPtr);
	private native long rpc(long endpointPtr, long queryPointer);
	private native void reply(long endpointPtr, long queryPointer, long replyPointer);
}
