package uk.ac.cam.tcs40.sbus.airs.sensor;

import uk.ac.cam.tcs40.sbus.SEndpoint;

public class AirsEndpoint extends SEndpoint {

	public enum TYPE { SInt, SText };
	private String m_SensorCode;
	private String m_ValueName;
	private UIHandler m_UIHandler;
	private TYPE m_ValueType;

	public AirsEndpoint(String endpointName, String endpointHash, String sensor, String valueName, TYPE valueType, UIHandler uiHandler) {
		super(endpointName, endpointHash);
		this.m_SensorCode = sensor;
		this.m_ValueName = valueName;
		this.m_ValueType = valueType;
		this.m_UIHandler = uiHandler;
	}
	
	public String getSensorCode() {
		return this.m_SensorCode;
	}
	
	public String getValueName() {
		return m_ValueName;
	}
	
	public TYPE getValueType() {
		return this.m_ValueType;
	}
	
	public UIHandler getUIHandler() {
		return this.m_UIHandler;
	}

}
