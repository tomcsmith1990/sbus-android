package uk.ac.cam.tcs40.sbus.mapper;

import uk.ac.cam.tcs40.sbus.SEndpoint;
import uk.ac.cam.tcs40.sbus.SNode;

public class MapEndpoint {

	private SEndpoint m_Endpoint;
	
	public MapEndpoint(SEndpoint ept) {
		this.m_Endpoint = ept;
	}

	/***
	 * Emits a map message to map components together.
	 * @param localAddress The address of the component on the phone.
	 * @param localEndpoint The endpoint of the component on the phone.
	 * @param peerAddress The address of the other component.
	 * @param peerEndpoint The endpoint of the other component.
	 */
	public void map(String localAddress, String localEndpoint, String peerAddress, String peerEndpoint) {
		this.m_Endpoint.endpointMap(localAddress);

		SNode node = this.m_Endpoint.createMessage("map");
		node.packString(localEndpoint, "endpoint");
		node.packString(peerAddress, "peer_address");
		node.packString(peerEndpoint, "peer_endpoint");
		node.packString("", "certificate");

		this.m_Endpoint.emit(node);
		
		this.m_Endpoint.endpointUnmap();
	}
}
