package uk.ac.cam.tcs40.sbus.mapper;

import uk.ac.cam.tcs40.sbus.SComponent;
import uk.ac.cam.tcs40.sbus.SComponent.EndpointType;
import uk.ac.cam.tcs40.sbus.SEndpoint;

public class AirsEndpointManager {

	private SComponent m_Component;
	private SEndpoint m_DataSinkEndpoint;
	private SEndpoint m_SubscriptionEndpoint;

	public AirsEndpointManager(SComponent component) {
		m_Component = component;
	}

	public SEndpoint addDataSinkEndpoint() {
		m_DataSinkEndpoint = m_Component.addEndpoint("AIRS", EndpointType.EndpointSink, "6187707D4CCE");
		return m_DataSinkEndpoint;
	}

	public SEndpoint addSubscriptionEndpoint() {
		m_SubscriptionEndpoint = m_Component.addEndpoint("airs_subscribe", EndpointType.EndpointSource, "F03F918E91A3");
		return m_SubscriptionEndpoint;
	}

}
