package uk.ac.cam.tcs40.sbus.mapper;

import uk.ac.cam.tcs40.sbus.SEndpoint;
import uk.ac.cam.tcs40.sbus.SNode;

public class RdcEndpoint {

	private SEndpoint m_Endpoint;
	
	public RdcEndpoint(SEndpoint ept) {
		this.m_Endpoint = ept;
	}
	
	/***
	 * Connects the component to an RDC.
	 * @param localAddress The address of the component on the phone.
	 * @param rdcAddress The adddress of the RDC to connect to.
	 */
	public void registerRdc(String localAddress, String rdcAddress) {
		this.m_Endpoint.endpointMap(localAddress, "register_rdc");
		
		SNode node = this.m_Endpoint.createMessage("event");
		node.packString(rdcAddress, "rdc_address");
		
		this.m_Endpoint.emit(node);
		
		this.m_Endpoint.endpointUnmap();
	}
}
