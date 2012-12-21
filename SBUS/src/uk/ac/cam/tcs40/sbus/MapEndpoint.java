package uk.ac.cam.tcs40.sbus;

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

		this.createMessage("map");
		this.packString(localEndpoint, "endpoint");
		this.packString(peerAddress, "peer_address");
		this.packString(peerEndpoint, "peer_endpoint");
		this.packString("", "certificate");

		this.emit();
		
		this.endpointUnmap();
	}
}
