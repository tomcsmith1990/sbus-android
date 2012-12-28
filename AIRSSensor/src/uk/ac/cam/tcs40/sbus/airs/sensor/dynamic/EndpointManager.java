package uk.ac.cam.tcs40.sbus.airs.sensor.dynamic;

import android.content.Context;
import uk.ac.cam.tcs40.sbus.FileBootloader;
import uk.ac.cam.tcs40.sbus.SComponent;
import uk.ac.cam.tcs40.sbus.SEndpoint;
import uk.ac.cam.tcs40.sbus.SMessage;
import uk.ac.cam.tcs40.sbus.SNode;

public class EndpointManager {

	private SComponent m_TargetComponent;
	private SComponent m_Component;
	private SEndpoint m_Endpoint;
	private Context m_Context;

	public EndpointManager(Context context, SComponent targetComponent) {
		this.m_Context = context;
		this.m_TargetComponent = targetComponent;
	}
	
	/**
	 * Creates and start a component which reads schemata and creates new endpoints on the AirsComponent.
	 */
	public void createComponent() {
		String cptFile = "CreateEpt.cpt";
		new FileBootloader(this.m_Context).store(cptFile);
		
		this.m_Component = new SComponent("create_ept", "creator");
		this.m_Endpoint = this.m_Component.addEndpointSink("addept", "E0D2B4C85BC6");
		//this.m_Component.addRDC("192.168.0.3:50123");
		this.m_Component.start(this.m_Context.getFilesDir() + "/" + cptFile, 44440, false);
		this.m_Component.setPermission("", "", true);
	}

	public SEndpoint createEndpoint(String endptname, String schema) {
		String hash = this.m_TargetComponent.declareSchema(schema);

		return this.m_TargetComponent.addEndpointSource(endptname, hash);
	}

	public SEndpoint readEndpoint() {
		if (this.m_Component == null) return null;
		
		SMessage message;
		SNode node;

		String endptname, schema, hash;

		message = this.m_Endpoint.receive();
		node = message.getTree();

		endptname = node.extractString("endptname");
		schema = node.extractString("schema");

		hash = this.m_TargetComponent.declareSchema(schema);

		return this.m_TargetComponent.addEndpointSource(endptname, hash);
	}
}
