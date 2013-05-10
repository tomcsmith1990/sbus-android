package uk.ac.cam.tcs40.sbus.mapper;

import uk.ac.cam.tcs40.sbus.Policy;

public class MapPolicy extends Policy {

	private final String m_LocalEndpoint;

	public MapPolicy(String localEndpoint, String remoteAddress, String remoteEndpoint, AIRS sensor, Condition condition, int value) {
		super(remoteAddress, remoteEndpoint);
		this.m_LocalEndpoint = localEndpoint;
		super.setSensor(sensor);
		super.setCondition(condition, value);
	}

	public String getLocalEndpoint() { return this.m_LocalEndpoint; }
}
