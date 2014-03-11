package uk.ac.cam.tcs40.sbus.mapper;

import uk.ac.cam.tcs40.sbus.SComponent;
import uk.ac.cam.tcs40.sbus.SEndpoint;
import uk.ac.cam.tcs40.sbus.SMessage;
import uk.ac.cam.tcs40.sbus.SNode;
import uk.ac.cam.tcs40.sbus.SComponent.EndpointType;

class ComponentPermissions {

	private SComponent m_Component;
	private SEndpoint m_PermissionsEndpoint;

	ComponentPermissions(SComponent component) {
		m_Component = component;
	}

	SEndpoint createPermissionsEndpoint() {
		m_PermissionsEndpoint = m_Component.addEndpoint("set_acl", EndpointType.EndpointSink, "6AF2ED96750B");
		return m_PermissionsEndpoint;
	}
	
	void unmapPermissionsEndpoint() {
		m_PermissionsEndpoint.unmap();
		m_PermissionsEndpoint = null;
	}
	
	void changePermissions() {
		String targetComponent, targetInstance, //targetAddress, targetEndpoint,
		remoteComponent, remoteInstance, 
		sourceComponent, sourceInstance;
		boolean allow;

		SMessage message;
		SNode snode;

		message = m_PermissionsEndpoint.receive();
		sourceComponent = message.getSourceComponent();
		sourceInstance = message.getSourceInstance();

		snode = message.getTree();

		targetComponent = snode.extractString("target_cpt");
		targetInstance = snode.extractString("target_inst");
		//targetAddress = snode.extractString("target_address");
		//targetEndpoint = snode.extractString("target_endpt");
		remoteComponent = snode.extractString("principal_cpt");
		remoteInstance = snode.extractString("principal_inst");
		allow = snode.extractBoolean("add_perm");

		message.delete();

		// Check that the component is setting a permission for itself.
		if (!targetComponent.equals(sourceComponent) || !targetInstance.equals(sourceInstance))
			return;

		if (targetComponent.equals("rdc")) {
			// local rule, handled by wrapper.
			m_Component.setPermission(remoteComponent, remoteInstance, allow);
		} else {
			Registration registration = RegistrationRepository.find(targetComponent, targetInstance);
			if (registration != null) {
				registration.addPermission(remoteComponent, remoteInstance, allow);
			}
		}
	}

}
