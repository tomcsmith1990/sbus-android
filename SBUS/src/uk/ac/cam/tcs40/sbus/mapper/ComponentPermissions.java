package uk.ac.cam.tcs40.sbus.mapper;

import uk.ac.cam.tcs40.sbus.SComponent;
import uk.ac.cam.tcs40.sbus.SEndpoint;
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

}
