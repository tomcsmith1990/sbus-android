package uk.ac.cam.tcs40.sbus.mapper;

public class Permission {

	private final String m_Component;
	private final String m_Instance;
	private final boolean m_Allow;

	public Permission(String component, String instance, boolean allow) {
		this.m_Component = component;
		this.m_Instance = instance;
		this.m_Allow = allow;
	}
	
	public boolean getPermission() {
		return this.m_Allow;
	}

	public boolean exists(String component, String instance) {
		return this.m_Component.equals(component) && this.m_Instance.equals(instance);
	}
}
