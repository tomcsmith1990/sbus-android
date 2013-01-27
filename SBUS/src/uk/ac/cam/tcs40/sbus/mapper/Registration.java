package uk.ac.cam.tcs40.sbus.mapper;

import java.util.Calendar;
import java.util.Date;
import java.util.LinkedList;
import java.util.List;

public class Registration {

	private final String m_Port, m_Component, m_Instance;
	private Date m_Date;
	private final List<Permission> m_Permissions;

	public Registration(String port, String component, String instance) {
		this.m_Port = port;
		this.m_Component = component;
		this.m_Instance = instance;
		update();
		this.m_Permissions = new LinkedList<Permission>();
	}
	
	public void addPermission(String component, String instance, boolean allow) {
		boolean exists = false;
		for (Permission p : this.m_Permissions) {
			
			if (p.exists(component, instance)) {
				exists = true;
				if (p.getPermission() != allow) {
					this.m_Permissions.remove(p);
					exists = false;
				}
			}
		}
		
		if (!exists)
			this.m_Permissions.add(new Permission(component, instance, allow));
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
