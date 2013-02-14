package uk.ac.cam.tcs40.sbus.mapper;

public class Permission {

	private final String m_RemoteComponent;
	private final String m_RemoteInstance;
	private final boolean m_Allow;

	public Permission(String remoteComponent, String remoteInstance, boolean allow) {
		this.m_RemoteComponent = remoteComponent;
		this.m_RemoteInstance = remoteInstance;
		this.m_Allow = allow;
	}

	public String getRemoteComponent() {
		return this.m_RemoteComponent;
	}

	public String getRemoteInstance() {
		return this.m_RemoteInstance;
	}

	public boolean getPermission() {
		return this.m_Allow;
	}

	public boolean isPermissionFor(String remoteComponent, String remoteInstance) {
		return getRemoteComponent().equals(remoteComponent) && getRemoteInstance().equals(remoteInstance);
	}
}
