package uk.ac.cam.tcs40.sbus.mapper;

import uk.ac.cam.tcs40.sbus.SComponent;
import uk.ac.cam.tcs40.sbus.SEndpoint;
import uk.ac.cam.tcs40.sbus.SComponent.EndpointType;

class PolicyDirectory {

	private SComponent m_Component;
	private SEndpoint m_PolicySink;

	PolicyDirectory(SComponent component) {
		m_Component = component;
	}

	SEndpoint addPolicySinkEndpoint() {
		m_PolicySink = m_Component.addEndpoint("map_policy", EndpointType.EndpointSink, "157EC474FA55");
		return m_PolicySink;
	}

}
