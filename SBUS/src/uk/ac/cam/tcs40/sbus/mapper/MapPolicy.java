package uk.ac.cam.tcs40.sbus.mapper;

public class MapPolicy {

	private final String m_LocalEndpoint;
	private final String m_RemoteAddress;
	private final String m_RemoteEndpoint;

	public MapPolicy(String localEndpoint, String remoteAddress, String remoteEndpoint) {
		this.m_LocalEndpoint = localEndpoint;
		this.m_RemoteAddress = remoteAddress;
		this.m_RemoteEndpoint = remoteEndpoint;
	}

	public String getLocalEndpoint() { return this.m_LocalEndpoint; }
	public String getRemoteAddress() { return this.m_RemoteAddress; }
	public String getRemoteEndpoint() { return this.m_RemoteEndpoint; }
}
