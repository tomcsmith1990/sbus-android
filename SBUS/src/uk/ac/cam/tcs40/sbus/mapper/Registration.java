package uk.ac.cam.tcs40.sbus.mapper;

import java.util.Calendar;
import java.util.Date;
import java.util.LinkedList;
import java.util.List;

import uk.ac.cam.tcs40.sbus.Policy;

public class Registration {

	private final String m_Port, m_Component, m_Instance;
	private Date m_LastCheckedAlive;
	private final List<Permission> m_Permissions;
	private final List<MapPolicy> m_MapPolicies;

	public Registration(String port, String component, String instance) {
		this.m_Port = port;
		this.m_Component = component;
		this.m_Instance = instance;
		update();
		this.m_Permissions = new LinkedList<Permission>();
		this.m_MapPolicies = new LinkedList<MapPolicy>();
	}
	
	public void addPermission(String remoteComponent, String remoteInstance, boolean allow) {
		boolean exists = false;
		for (Permission p : this.m_Permissions) {
			
			if (p.isPermissionFor(remoteComponent, remoteInstance)) {
				exists = true;
				if (p.getPermission() != allow) {
					this.m_Permissions.remove(p);
					exists = false;
				}
			}
		}
		
		if (!exists)
			this.m_Permissions.add(new Permission(remoteComponent, remoteInstance, allow));
	}
	
	public void addMapPolicy(MapPolicy policy) {
		this.m_MapPolicies.add(policy);
	}
	
	public void removeMapPolicy(MapPolicy policy) {
		for (MapPolicy existingPolicy : this.m_MapPolicies) {
			if (existingPolicy.getLocalEndpoint().equals(policy.getLocalEndpoint()) && 
					existingPolicy.getRemoteAddress().equals(policy.getRemoteAddress()) && 
					existingPolicy.getRemoteEndpoint().equals(policy.getRemoteEndpoint())) {
				this.m_MapPolicies.remove(existingPolicy);
			}
		}
	}
	
	public List<MapPolicy> getMapPolicies() {
		return this.m_MapPolicies;
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
	
	public Date getLastCheckedAlive() {
		return this.m_LastCheckedAlive;
	}
	
	public void update() {
		this.m_LastCheckedAlive = Calendar.getInstance().getTime();
	}
}
