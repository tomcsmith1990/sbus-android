package uk.ac.cam.tcs40.sbus;

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;

public class SNode {

	private long m_NodePtr;
	
	SNode(long ptr) {
		/*
		 *  Note - no modifier means package only access.
		 *  Can only create SNode through:
		 *  	- SEndpoint.createMessage()
		 *  	- SEndpoint.receive().getTree()
		 *  	- SNode.find()
		 */
		this.m_NodePtr = ptr;
	}
	
	long getPointer() {
		// Package only access- called from SEndpoint.emit()
		return this.m_NodePtr;
	}
	
	/**
	 * 
	 * @return The number of items in this snode.
	 */
	public int count() {
		return count(this.m_NodePtr);
	}
	
	/**
	 * Check whether a field exists in the SNode.
	 * @param name The field to check.
	 * @return Whether the field exists in the SNode.
	 */
	public boolean exists(String name) {
		return (exists(this.m_NodePtr, name) == 1);
	}
	
	public SNode find(String name) {
		long ptr = find(this.m_NodePtr, name);
		if (ptr == 0) return null;
		else return new SNode(ptr);
	}
	
	/**
	 * Delete the native representation of this snode.
	 */
	public void delete() {
		delete(this.m_NodePtr);
	}
	
	public void packBoolean(boolean b) {
		packBoolean(b, null);
	}
	
	public void packBoolean(boolean b, String name) {
		packBoolean(this.m_NodePtr, b, name);
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
		DateFormat format = new SimpleDateFormat("dd/MM/yyyy,HH:mm:ss", Locale.UK);
		packClock(this.m_NodePtr, format.format(d), name);
	}
	
	public boolean extractBoolean(String name) {
		return (extractBoolean(this.m_NodePtr, name) == 1);
	}
	
	public int extractInt(String name) {
		return extractInt(this.m_NodePtr, name);
	}
	
	public double extractDouble(String name) {
		return extractDouble(this.m_NodePtr, name);
	}
	
	public String extractString(String name) {
		return extractString(this.m_NodePtr, name);
	}
	
	public SNode extractItem(int item) {
		long ptr = extractItem(this.m_NodePtr, item);
		return new SNode(ptr);
	}
	
	public SNode extractItem(String item) {
		long ptr = extractItemByName(this.m_NodePtr, item);
		return new SNode(ptr);
	}
	
	private native void packBoolean(long nodePtr, boolean b, String name);
	private native void packInt(long nodePtr, int n, String name);
	private native void packDouble(long nodePtr, double d, String name);
	private native void packString(long nodePtr, String s, String name);
	private native void packClock(long nodePtr, String date, String name);
	
	private native int extractBoolean(long nodePtr, String name);
	private native int extractInt(long nodePtr, String name);
	private native double extractDouble(long nodePtr, String name);
	private native String extractString(long nodePtr, String name);
	
	private native long extractItem(long nodePtr, int item);
	private native long extractItemByName(long nodePtr, String item);
	
	private native int exists(long nodePtr, String name);
	private native long find(long nodePtr, String name);
	private native int count(long nodePtr);
	private native void delete(long nodePtr);
}
