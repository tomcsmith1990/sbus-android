package uk.ac.cam.tcs40.sbus.mapper;

public class MapPolicy {

	private final String m_LocalEndpoint;
	private final String m_RemoteComponent;
	private final String m_RemoteEndpoint;

	public MapPolicy(String localEndpoint, String remoteComponent, String remoteEndpoint) {
		this.m_LocalEndpoint = localEndpoint;
		this.m_RemoteComponent = remoteComponent;
		this.m_RemoteEndpoint = remoteEndpoint;
	}

	public String getLocalEndpoint() { return this.m_LocalEndpoint; }
	public String getRemoteComponent() { return this.m_RemoteComponent; }
	public String getRemoteEndpoint() { return this.m_RemoteEndpoint; }

}
