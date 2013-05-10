package uk.ac.cam.tcs40.sbus;

public class Policy {

	private final String m_RemoteAddress;
	private final String m_RemoteEndpoint;

	private AIRS m_Sensor = AIRS.NONE;
	private Condition m_Condition = Condition.NONE;
	private int m_Value = 0;

	public enum AIRS { NONE, RANDOM, WIFI };
	public enum Condition { NONE, EQUAL, LESS_THAN, GREATER_THAN, LESS_THAN_EQUAL, GREATER_THAN_EQUAL };

	public static String sensorCode(AIRS sensor) {
		switch (sensor) {
		case RANDOM:
			return "Rd";
		default:
			return null;
		}
	}

	public Policy(String remoteAddress, String remoteEndpoint) {
		this.m_RemoteAddress = remoteAddress;
		this.m_RemoteEndpoint = remoteEndpoint;
	}

	public String getRemoteAddress() { return this.m_RemoteAddress; }
	public String getRemoteEndpoint() { return this.m_RemoteEndpoint; }

	public AIRS getSensor() { return m_Sensor; }
	public Condition getCondition() { return m_Condition; }
	public int getValue() { return m_Value; }

	public void setSensor(AIRS sensor) {
		m_Sensor = sensor;
	}

	public void setCondition(Condition condition, int value) {
		m_Condition = condition;
		m_Value = value;
	}


}
