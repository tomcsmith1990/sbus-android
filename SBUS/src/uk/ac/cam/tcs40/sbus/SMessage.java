package uk.ac.cam.tcs40.sbus;

public class SMessage {

	private long m_MessagePtr;
	
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
	 * Delete the native representation of this message.
	 */
	public void delete() {
		delete(this.m_MessagePtr);
	}
	
	private native long getTree(long ptr);
	private native void delete(long ptr);
}
