package uk.ac.cam.tcs40.sbus.mapper;

import uk.ac.cam.tcs40.sbus.SComponent;
import uk.ac.cam.tcs40.sbus.SEndpoint;
import uk.ac.cam.tcs40.sbus.SComponent.EndpointType;

class ComponentRegister {

	private SComponent m_Component;
	private SEndpoint m_RegistrationEndpoint;

	ComponentRegister(SComponent component) {
		m_Component = component;
	}

	SEndpoint createRegistrationEndpoint() {
		m_RegistrationEndpoint = m_Component.addEndpoint("register", EndpointType.EndpointSink, "B3572388E4A4");
		return m_RegistrationEndpoint;
	}

	void unmapRegistrationEndpoint() {
		m_RegistrationEndpoint.unmap();
		m_RegistrationEndpoint = null;		
	}

}
