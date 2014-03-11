package uk.ac.cam.tcs40.sbus.mapper;

import uk.ac.cam.tcs40.sbus.Policy;

public class MapPolicy extends Policy {

	private final String m_LocalEndpoint;

	public MapPolicy(String localEndpoint, String remoteAddress, String remoteEndpoint, AIRS sensor, Condition condition, int value, boolean create) {
		super(remoteAddress, remoteEndpoint, sensor, condition, value, create);
		this.m_LocalEndpoint = localEndpoint;
	}

	public String getLocalEndpoint() { return this.m_LocalEndpoint; }
}
