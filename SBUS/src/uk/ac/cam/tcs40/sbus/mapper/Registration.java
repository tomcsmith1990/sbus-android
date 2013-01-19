package uk.ac.cam.tcs40.sbus.mapper;

import java.util.Calendar;
import java.util.Date;

public class Registration {

	private String m_Port;
	private Date m_Date;

	public Registration(String port) {
		this.m_Port = port;
		update();
	}
	
	public String getPort() {
		return m_Port;
	}
	
	public Date getDate() {
		return this.m_Date;
	}
	
	public void update() {
		this.m_Date = Calendar.getInstance().getTime();
	}
}
