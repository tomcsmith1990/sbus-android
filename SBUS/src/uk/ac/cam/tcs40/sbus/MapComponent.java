package uk.ac.cam.tcs40.sbus;

public class MapComponent extends SComponent {

	public MapComponent(String componentName, String instanceName) {
		super(componentName, instanceName);
	}

	public void map(String localAddress, String localEndpoint, String peerAddress, String peerEndpoint) {
		this.endpointMap(localAddress);

		this.createMessage("map");
		this.packString(localEndpoint, "endpoint");
		this.packString(peerAddress, "peer_address");
		this.packString(peerEndpoint, "peer_endpoint");
		this.packString("", "certificate");

		this.emit();		
	}
}
