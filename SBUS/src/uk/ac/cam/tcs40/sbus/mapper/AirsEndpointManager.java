package uk.ac.cam.tcs40.sbus.mapper;

import java.util.LinkedList;
import java.util.List;

import uk.ac.cam.tcs40.sbus.SComponent;
import uk.ac.cam.tcs40.sbus.SNode;
import uk.ac.cam.tcs40.sbus.SComponent.EndpointType;
import uk.ac.cam.tcs40.sbus.SEndpoint;

public class AirsEndpointManager {

	private SComponent m_Component;
	private SEndpoint m_DataSinkEndpoint;
	private SEndpoint m_SubscriptionEndpoint;
	private final List<String> m_AirsSubscriptions = new LinkedList<String>();

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
	
	public void unmapDataSinkEndpoint() {
		m_DataSinkEndpoint.unmap();
		m_DataSinkEndpoint = null;
	}
	
	public void unmapSubscriptionEndpoint() {
		m_SubscriptionEndpoint.unmap();
		m_SubscriptionEndpoint.unmap();
	}
	
	public void subscribeToAIRS(String airsAddress, String sensorCode) {
		if (airsAddress == null || sensorCode == null) 
			return;

		if (!m_AirsSubscriptions.contains(sensorCode)) {
			m_SubscriptionEndpoint.map(airsAddress, "subscribe");
			SNode subscription = m_SubscriptionEndpoint.createMessage("subscription");
			subscription.packString(sensorCode, "sensor");
			m_SubscriptionEndpoint.emit(subscription);
			m_SubscriptionEndpoint.unmap();

			m_AirsSubscriptions.add(sensorCode);
		}
	}

}
