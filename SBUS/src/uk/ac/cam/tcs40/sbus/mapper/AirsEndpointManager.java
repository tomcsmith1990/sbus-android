package uk.ac.cam.tcs40.sbus.mapper;

import java.util.LinkedList;
import java.util.List;

import uk.ac.cam.tcs40.sbus.SComponent;
import uk.ac.cam.tcs40.sbus.SNode;
import uk.ac.cam.tcs40.sbus.SComponent.EndpointType;
import uk.ac.cam.tcs40.sbus.SEndpoint;

class AirsEndpointManager {

	private SComponent m_Component;
	private SEndpoint m_DataSinkEndpoint;
	private SEndpoint m_SubscriptionEndpoint;
	private final List<String> m_AirsSubscriptions = new LinkedList<String>();
	private String m_ServerAddress;

	AirsEndpointManager(SComponent component) {
		m_Component = component;
	}

	SEndpoint addDataSinkEndpoint() {
		m_DataSinkEndpoint = m_Component.addEndpoint("AIRS", EndpointType.EndpointSink, "6187707D4CCE");
		return m_DataSinkEndpoint;
	}

	void addSubscriptionEndpoint() {
		m_SubscriptionEndpoint = m_Component.addEndpoint("airs_subscribe", EndpointType.EndpointSource, "F03F918E91A3");
	}
	
	void unmapDataSinkEndpoint() {
		m_DataSinkEndpoint.unmap();
		m_DataSinkEndpoint = null;
	}
	
	void unmapSubscriptionEndpoint() {
		m_SubscriptionEndpoint.unmap();
		m_SubscriptionEndpoint = null;
	}
	
	void subscribeToAIRS(String sensorCode) {
		if (m_ServerAddress == null || sensorCode == null) 
			return;

		if (!m_AirsSubscriptions.contains(sensorCode)) {
			m_SubscriptionEndpoint.map(m_ServerAddress, "subscribe");
			SNode subscription = m_SubscriptionEndpoint.createMessage("subscription");
			subscription.packString(sensorCode, "sensor");
			m_SubscriptionEndpoint.emit(subscription);
			m_SubscriptionEndpoint.unmap();

			m_AirsSubscriptions.add(sensorCode);
		}
	}

	void setAirsServerAddress(String airsServerAddress) {
		m_ServerAddress = airsServerAddress;		
	}

}
