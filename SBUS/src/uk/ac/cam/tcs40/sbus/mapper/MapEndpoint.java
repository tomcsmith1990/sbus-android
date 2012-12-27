package uk.ac.cam.tcs40.sbus.mapper;

import uk.ac.cam.tcs40.sbus.SEndpoint;
import uk.ac.cam.tcs40.sbus.SNode;

public class MapEndpoint extends SEndpoint {

	public MapEndpoint(String endpointName, String endpointHash) {
		super(endpointName, endpointHash);
	}

	/***
	 * Emits a map message to map components together.
	 * @param localAddress The address of the component on the phone.
	 * @param localEndpoint The endpoint of the component on the phone.
	 * @param peerAddress The address of the other component.
	 * @param peerEndpoint The endpoint of the other component.
	 */
	public void map(String localAddress, String localEndpoint, String peerAddress, String peerEndpoint) {
		this.endpointMap(localAddress);

		SNode node = this.createMessage("map");
		node.packString(localEndpoint, "endpoint");
		node.packString(peerAddress, "peer_address");
		node.packString(peerEndpoint, "peer_endpoint");
		node.packString("", "certificate");

		this.emit(node);
		
		this.endpointUnmap();
	}
}
