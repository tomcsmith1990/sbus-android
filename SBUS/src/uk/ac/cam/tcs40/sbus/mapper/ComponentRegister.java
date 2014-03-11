package uk.ac.cam.tcs40.sbus.mapper;

import uk.ac.cam.tcs40.sbus.SComponent;
import uk.ac.cam.tcs40.sbus.SEndpoint;
import uk.ac.cam.tcs40.sbus.SMessage;
import uk.ac.cam.tcs40.sbus.SNode;
import uk.ac.cam.tcs40.sbus.SComponent.EndpointType;

class ComponentRegister {

	private SComponent m_Component;
	private SEndpoint m_RegistrationEndpoint;
	private String m_AirsServerAddress;

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
	
	String getAirsServerAddress() { 
		return m_AirsServerAddress;
	}
	
	void acceptRegistration(String phoneIp) {
		String address, host, port, sourceComponent, sourceInstance;
		boolean register;
		SMessage message;
		SNode snode;

		message = m_RegistrationEndpoint.receive();
		sourceComponent = message.getSourceComponent();
		sourceInstance = message.getSourceInstance();

		snode = message.getTree();
		address = snode.extractString("address");
		register = snode.extractBoolean("arrived");

		message.delete();

		host = address.split(":")[0];
		port = address.split(":")[1];

		// Check this is a component on the phone.
		if (!host.equals(phoneIp) && !host.equals("127.0.0.1") && !host.equals("") && !host.equals("10.0.2.15"))
			return;

		if (register) {

			if (sourceComponent.equals("AirsSensor")) {
				m_AirsServerAddress = ":" + port;
				PMCActivity.addStatus("Found AIRS at :" + port);
				return;
				//subscribeToAIRS("WC");
			}

			Registration registration = RegistrationRepository.add(port, sourceComponent, sourceInstance);
			if (registration != null) {
				PMCActivity.addStatus("Registered component " + sourceComponent + " instance " + sourceInstance + ", at :" + port);
			} else {
				PMCActivity.addStatus("Attempting to register already registered component " + sourceComponent + ":" + sourceInstance);
			}

		} else {
			Registration deregistration = RegistrationRepository.remove(port);
			PMCActivity.addStatus("Deregistered component " + deregistration.getComponentName() + ":" + deregistration.getInstanceName() + " at :" + port);
		}
	}

}
