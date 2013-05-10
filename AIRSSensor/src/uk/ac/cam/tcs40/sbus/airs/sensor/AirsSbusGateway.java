package uk.ac.cam.tcs40.sbus.airs.sensor;

import java.io.IOException;
import java.util.LinkedList;
import java.util.List;

import com.airs.platform.EventComponent;
import com.airs.platform.Server;

import android.content.Context;
import android.util.Log;
import uk.ac.cam.tcs40.sbus.FileBootloader;
import uk.ac.cam.tcs40.sbus.Multiplex;
import uk.ac.cam.tcs40.sbus.SComponent;
import uk.ac.cam.tcs40.sbus.SComponent.EndpointType;
import uk.ac.cam.tcs40.sbus.SEndpoint;
import uk.ac.cam.tcs40.sbus.SMessage;
import uk.ac.cam.tcs40.sbus.SNode;
import uk.ac.cam.tcs40.sbus.airs.sensor.dynamic.EndpointManager;

public class AirsSbusGateway {

	private SComponent m_Component;
	private SEndpoint m_SubscriptionEndpoint;
	private Server m_Server;

	public final List<String> m_Subscriptions = new LinkedList<String>();

	private SensorReadingActivity m_Activity;
	private Context m_Context;

	private final String m_CptFile = "AirsSensor.cpt";

	public AirsSbusGateway(SensorReadingActivity activity) {
		m_Activity = activity;
		m_Context = activity.getApplicationContext();

		// Store the component files used.
		storeComponentFiles();

		// Create the component.
		m_Component = new SComponent("AirsSensor", "airs");
		m_SubscriptionEndpoint = m_Component.addEndpoint("subscribe", EndpointType.EndpointSink, "F03F918E91A3");

		// Register RDC if it is available.
		//this.m_Component.addRDC("192.168.0.3:50123");

		// Start the component, load the .cpt file.
		m_Component.start(m_Context.getFilesDir() + "/" + m_CptFile, -1, true);
		m_Component.setPermission("AirsConsumer", "", true);
	}

	public void subscribe(String sensorCode) {
		if (this.m_Server == null) return;

		if (!m_Subscriptions.contains(sensorCode)) {
			this.m_Server.subscribe(sensorCode);
			m_Subscriptions.add(sensorCode);
			Log.i("AIRS", "Subscribe to " + sensorCode);
		}
	}

	public void stop() {
		this.m_Component.delete();

		try {
			if (this.m_Server != null)
				this.m_Server.stop();
		} catch (IOException e) {
		}
	}

	public void startReadings() {
		// Set up the stuff for the AIRS server.
		EventComponent eventComponent = new EventComponent();
		AirsAcquisition acquisition;
		AirsDiscovery discovery = new AirsDiscovery(eventComponent, this.m_Activity);

		EndpointManager endpointManager = new EndpointManager(this.m_Context, this.m_Component);
		acquisition = new AirsAcquisition(eventComponent, endpointManager);

		this.m_Server = new Server(9000, eventComponent, acquisition, discovery);

		try {
			this.m_Activity.setStatusText("waiting for AIRS to connect");						
			// Start a server waiting for AIRS Remote to connect.
			this.m_Server.startConnection();

			this.m_Activity.setStatusText("loading AIRS sensor list");						
			//discovery.loadSensorsFromFile();

			this.m_Activity.setStatusText("ready for AIRS subscriptions");

			Multiplex multi = m_Component.getMultiplex();
			multi.add(m_SubscriptionEndpoint);

			while (m_Component != null) {

				// Wait until a message is ready.
				try {
					multi.waitForMessage();
				} catch (Exception e) {
					// Exception thrown if waitForMessage() returns with an endpoint not on the component which owns the Multiplex.
					break;
				}

				SMessage message = m_SubscriptionEndpoint.receive();
				SNode node = message.getTree();

				String sensorCode = node.extractString("sensor");

				message.delete();

				subscribe(sensorCode);
			}

		} catch (IOException e) {

		}
	}

	private void storeComponentFiles() {
		// Create a FileBootloader to store our component file.
		new FileBootloader(this.m_Context).store(m_CptFile);
	}
}
