package uk.ac.cam.tcs40.sbus.mapper;

import uk.ac.cam.tcs40.sbus.SComponent;
import uk.ac.cam.tcs40.sbus.SEndpoint;
import uk.ac.cam.tcs40.sbus.SMessage;
import uk.ac.cam.tcs40.sbus.SNode;
import uk.ac.cam.tcs40.sbus.Policy.AIRS;
import uk.ac.cam.tcs40.sbus.Policy.Condition;
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
	
	void unmapPolicySinkEndpoint() {
		m_PolicySink.unmap();
		m_PolicySink = null;
	}
	
	MapPolicy readPolicy() throws Exception {
		
		SMessage message = m_PolicySink.receive();
		SNode snode = message.getTree();

		boolean create = snode.extractBoolean("create");
		String remoteAddress = snode.extractString("peer_address");
		String remoteEndpoint = snode.extractString("peer_endpoint");
		String localEndpoint = snode.extractString("endpoint");
		int sensor = snode.extractInt("sensor");
		int condition = snode.extractInt("condition");
		int value = snode.extractInt("value");

		MapPolicy policy = new MapPolicy(localEndpoint, remoteAddress, remoteEndpoint, AIRS.values()[sensor], Condition.values()[condition], value, create);
		
		Registration registration = RegistrationRepository.find(message.getSourceComponent(), message.getSourceInstance());
		
		message.delete();
		
		if (registration != null) {

			if (create) {
				registration.addMapPolicy(policy);
				PMCActivity.addStatus("Adding map policy: " + remoteAddress + " for component " + registration.getComponentName());
			}
			else {
				registration.removeMapPolicy(policy);
				PMCActivity.addStatus("Removing map policy: " + remoteAddress + " for component " + registration.getComponentName());
			}
			
			return policy;
		}
		
		throw new Exception("Could not find registration");
	}

}
