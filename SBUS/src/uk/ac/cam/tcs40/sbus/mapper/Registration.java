package uk.ac.cam.tcs40.sbus.mapper;

public class Registration {

	private String m_Port;

	public Registration(String port) {
		this.m_Port = port;
	}
	
	public String getPort() {
		return m_Port;
	}
}
