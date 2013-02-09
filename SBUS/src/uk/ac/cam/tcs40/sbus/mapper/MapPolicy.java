package uk.ac.cam.tcs40.sbus.mapper;

public class MapPolicy {

	private final String m_LocalEndpoint;
	private final String m_PeerComponent;
	private final String m_PeerEndpoint;

	public MapPolicy(String localEndpoint, String peerComponent, String peerEndpoint) {
		this.m_LocalEndpoint = localEndpoint;
		this.m_PeerComponent = peerComponent;
		this.m_PeerEndpoint = peerEndpoint;
	}

	public String getLocalEndpoint() { return this.m_LocalEndpoint; }
	public String getPeerComponent() { return this.m_PeerComponent; }
	public String getPeerEndpoint() { return this.m_PeerEndpoint; }

}
