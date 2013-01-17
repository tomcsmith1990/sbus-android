package uk.ac.cam.tcs40.sbus;

public class SMessage {

	private long m_MessagePtr;
	
	private String m_SourceCpt;
	private String m_SourceInst;
	private String m_SourceEpt;
	
	SMessage(long ptr) {
		// Note - no modifier means package only access.
		// Can only create by receiving a message.
		this.m_MessagePtr = ptr;
	}
	
	/**
	 * 
	 * @return The message for extracting values.
	 */
	public SNode getTree() {
		long ptr = getTree(this.m_MessagePtr);
		return new SNode(ptr);
	}
	
	/**
	 * 
	 * @return The source component of this message.
	 */
	public String getSourceComponent() {
		if (this.m_SourceCpt == null)
			this.m_SourceCpt = getSourceComponent(this.m_MessagePtr);
		
		return this.m_SourceCpt;
	}
	
	/**
	 * 
	 * @return The source instance of this message.
	 */
	public String getSourceInstance() {
		if (this.m_SourceInst == null)
			this.m_SourceInst = getSourceEndpoint(this.m_MessagePtr);
		
		return this.m_SourceInst;
	}
	
	/**
	 * 
	 * @return The source endpoint of this message.
	 */
	public String getSourceEndpoint() {
		if (this.m_SourceEpt == null)
			this.m_SourceEpt = getSourceEndpoint(this.m_MessagePtr);
		
		return this.m_SourceEpt;
	}
	
	/**
	 * Delete the native representation of this message.
	 */
	public void delete() {
		delete(this.m_MessagePtr);
	}
	
	private native long getTree(long ptr);
	private native String getSourceComponent(long ptr);
	private native String getSourceInstance(long ptr);
	private native String getSourceEndpoint(long ptr);
	private native void delete(long ptr);
}
