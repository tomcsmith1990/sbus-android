package uk.ac.cam.tcs40.sbus.mapper;

import java.util.Calendar;
import java.util.Date;

public class Registration {

	private String m_Port, m_Component, m_Instance;
	private Date m_Date;

	public Registration(String port, String component, String instance) {
		this.m_Port = port;
		this.m_Component = component;
		this.m_Instance = instance;
		update();
	}
	
	public String getPort() {
		return this.m_Port;
	}
	
	public String getComponentName() {
		return this.m_Component;
	}
	
	public String getInstanceName() {
		return this.m_Instance;
	}
	
	public Date getDate() {
		return this.m_Date;
	}
	
	public void update() {
		this.m_Date = Calendar.getInstance().getTime();
	}
}
