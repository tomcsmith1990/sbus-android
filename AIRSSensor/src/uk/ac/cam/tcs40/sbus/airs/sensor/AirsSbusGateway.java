package uk.ac.cam.tcs40.sbus.airs.sensor;

import java.io.IOException;

import com.airs.platform.EventComponent;
import com.airs.platform.Server;

import android.content.Context;
import uk.ac.cam.tcs40.sbus.FileBootloader;
import uk.ac.cam.tcs40.sbus.SComponent;
import uk.ac.cam.tcs40.sbus.airs.sensor.dynamic.EndpointManager;

public class AirsSbusGateway {

	private SComponent m_Component;
	private Server m_Server;

	private SensorReadingActivity m_Activity;
	private Context m_Context;

	public AirsSbusGateway(SensorReadingActivity activity) {
		this.m_Activity = activity;
		this.m_Context = activity.getApplicationContext();

		// Store the component files used.
		storeComponentFiles();

		// Create the component.
		this.m_Component = new SComponent("AirsSensor", "airs");

		String cptFile;

		// The component file to use.
		cptFile = "AirsSensorDynamic.cpt";

		// Register RDC if it is available.
		//this.m_Component.addRDC("192.168.0.3:50123");

		// Start the component, load the .cpt file.
		this.m_Component.start(this.m_Context.getFilesDir() + "/" + cptFile, -1, true);
		this.m_Component.setPermission("AirsConsumer", "", true);
	}

	public void subscribe(String sensorCode) {
		if (this.m_Server == null) return;

		this.m_Server.subscribe(sensorCode);
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

		} catch (IOException e) {

		}
	}

	private void storeComponentFiles() {
		// Create a FileBootloader to store our component file.
		new FileBootloader(this.m_Context).store("AirsSensorDynamic.cpt");
	}
}
