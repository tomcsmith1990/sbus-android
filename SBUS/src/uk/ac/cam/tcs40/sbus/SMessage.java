package uk.ac.cam.tcs40.sbus;

public class SMessage {

	private long m_MessagePtr;
	
	public SMessage(long ptr) {
		this.m_MessagePtr = ptr;
	}
	
	public SNode getTree() {
		long ptr = getTree(this.m_MessagePtr);
		return new SNode(ptr);
	}
	
	public void delete() {
		delete(this.m_MessagePtr);
	}
	
	private native long getTree(long ptr);
	private native void delete(long ptr);
}
