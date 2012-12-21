package uk.ac.cam.tcs40.sbus;

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Date;

public class SEndpoint {
	
	private String m_EndpointName;
	private String m_EndpointHash;
	private long m_EndpointPointer;
	private long m_MessagePointer;
	
	public SEndpoint(String endpointName, String endpointHash) {
		this.m_EndpointName = endpointName;
		this.m_EndpointHash = endpointHash;	
	}
	
	public String getEndpointName() {
		return this.m_EndpointName;
	}
	
	public String getEndpointHash() {
		return this.m_EndpointHash;
	}
	
	public void setPointer(long ptr) {
		m_EndpointPointer = ptr;
	}

	public void createMessage(String messageType) {
		this.m_MessagePointer = createMessage(m_EndpointPointer, messageType);
	}
	private native long createMessage(long endpointPtr, String messageType);
	
	public String emit() {
		return emit(m_EndpointPointer, m_MessagePointer);
	}
	private native String emit(long endpointPtr, long messagePtr);

	public String endpointMap(String address) {
		return endpointMap(m_EndpointPointer, address);
	}
	private native String endpointMap(long endpointPtr, String address);
	
	public void endpointUnmap() {
		endpointUnmap(m_EndpointPointer);
	}
	private native void endpointUnmap(long endpointPtr);

	
	public void packInt(int n) { packInt(n, null); }
	public void packInt(int n, String name) {
		packInt(m_MessagePointer, n, name);
	}
	public native void packInt(long messagePtr, int n, String name);

	public void packString(String s) { packString(s, null); }
	public void packString(String s, String name) {
		packString(m_MessagePointer, s, name);
	}
	private native void packString(long messagePtr, String s, String name);

	public void packTime(Date d) { packTime(d, null); }
	public void packTime(Date d, String name) {
		DateFormat format = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss+00");
		packClock(m_MessagePointer, format.format(d), name);
	}
	private native void packClock(long messagePtr, String date, String name);
}
