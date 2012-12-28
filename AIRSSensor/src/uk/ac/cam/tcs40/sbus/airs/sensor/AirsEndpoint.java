package uk.ac.cam.tcs40.sbus.airs.sensor;

import uk.ac.cam.tcs40.sbus.SEndpoint;

public class AirsEndpoint {

	private SEndpoint m_Endpoint;
	
	public enum TYPE { SInt, SText };
	private String m_SensorCode;
	private UIHandler m_UIHandler;
	private TYPE m_ValueType;

	public AirsEndpoint(SEndpoint ept, String sensor, UIHandler uiHandler) {
		this.m_Endpoint = ept;
		this.m_SensorCode = sensor;
		this.m_UIHandler = uiHandler;
	}
	
	public AirsEndpoint(SEndpoint ept, String sensor, TYPE valueType, UIHandler uiHandler) {
		this.m_Endpoint = ept;
		this.m_SensorCode = sensor;
		this.m_ValueType = valueType;
		this.m_UIHandler = uiHandler;
	}
	
	public SEndpoint getEndpoint() {
		return this.m_Endpoint;
	}
	
	public String getSensorCode() {
		return this.m_SensorCode;
	}
		
	public TYPE getValueType() {
		return this.m_ValueType;
	}
	
	public UIHandler getUIHandler() {
		return this.m_UIHandler;
	}

}
