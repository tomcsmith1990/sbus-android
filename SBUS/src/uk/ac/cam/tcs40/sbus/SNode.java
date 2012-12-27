package uk.ac.cam.tcs40.sbus;

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Date;

public class SNode {

	private long m_NodePtr;
	
	public SNode(long ptr) {
		this.m_NodePtr = ptr;
	}
	
	public long getPointer() {
		return this.m_NodePtr;
	}
	
	public void delete() {
		delete(this.m_NodePtr);
	}
	
	public void packInt(int n) { 
		packInt(n, null); 
	}
	public void packInt(int n, String name) {
		packInt(this.m_NodePtr, n, name);
	}
	
	public void packDouble(double d) { 
		packDouble(d, null); 
	}
	public void packDouble(double d, String name) {
		packDouble(this.m_NodePtr, d, name);
	}

	public void packString(String s) { 
		packString(s, null); 
	}
	public void packString(String s, String name) {
		packString(this.m_NodePtr, s, name);
	}

	public void packTime(Date d) { 
		packTime(d, null);
	}
	public void packTime(Date d, String name) {
		DateFormat format = new SimpleDateFormat("dd/MM/yyyy,HH:mm:ss");
		packClock(this.m_NodePtr, format.format(d), name);
	}
	
	private native void packInt(long messagePtr, int n, String name);
	private native void packDouble(long messagePtr, double d, String name);
	private native void packString(long messagePtr, String s, String name);
	private native void packClock(long messagePtr, String date, String name);
	
	private native void delete(long messagePtr);
}
