package uk.ac.cam.tcs40.sbus.airs.sensor;

import java.io.IOException;
import java.util.List;

import com.airs.platform.EventComponent;
import com.airs.platform.Server;

import android.content.Context;
import uk.ac.cam.tcs40.sbus.FileBootloader;
import uk.ac.cam.tcs40.sbus.SComponent;
import uk.ac.cam.tcs40.sbus.SComponent.EndpointType;
import uk.ac.cam.tcs40.sbus.airs.sensor.AirsEndpoint.TYPE;
import uk.ac.cam.tcs40.sbus.airs.sensor.dynamic.EndpointManager;
import uk.ac.cam.tcs40.sbus.airs.sensor.dynamic.UIManager;

public class AirsSbusGateway {

	/*** CONFIGURATION ***/
	final boolean m_LiveReadings = true;
	final boolean m_DynamicEndpoints = true;

	private SComponent m_Component;
	private Server m_Server;
	private SensorReadingDB m_AirsDb;

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
		if (m_DynamicEndpoints) {
			cptFile = "AirsSensorDynamic.cpt";
		} else {
			cptFile = "AirsSensor.cpt";
			createEndpoints();
		}

		// Register RDC if it is available.
		this.m_Component.addRDC("192.168.0.3:50123");

		// Start the component, load the .cpt file.
		this.m_Component.start(this.m_Context.getFilesDir() + "/" + cptFile, 44445, true);
		this.m_Component.setPermission("AirsConsumer", "", true);
	}

	public void startReadings() {
		if (this.m_LiveReadings) {
			startLiveReadings();
		} else {
			// Create threads for relevant sensors.
			for (String sensorCode : AirsEndpointRepository.getSensorCodes())
				new Thread(new DBReadingHandler(this.m_AirsDb, sensorCode)).start();
		}
	}

	public void subscribe(String sensorCode) {
		if (this.m_Server == null) return;
		
		this.m_Server.subscribe(sensorCode);
	}

	public void stop() {
		this.m_Component.delete();
	}

	private void startLiveReadings() {
		// Set up the stuff for the AIRS server.
		EventComponent eventComponent = new EventComponent();
		AirsAcquisition acquisition;
		AirsDiscovery discovery = new AirsDiscovery(eventComponent, this.m_Activity);

		if (this.m_DynamicEndpoints) {
			EndpointManager endpointManager = new EndpointManager(this.m_Context, this.m_Component);
			acquisition = new AirsAcquisition(eventComponent, endpointManager);
		} else {
			acquisition = new AirsAcquisition(eventComponent, null);
		}

		this.m_Server = new Server(9000, eventComponent, acquisition, discovery);

		try {
			this.m_Activity.setStatusText("waiting for AIRS to connect");						
			// Start a server waiting for AIRS Remote to connect.
			this.m_Server.startConnection();
			
			this.m_Activity.setStatusText("loading AIRS sensor list");						
			discovery.loadSensorsFromFile();
			
			this.m_Activity.setStatusText("ready for AIRS subscriptions");

			if (!this.m_DynamicEndpoints) {
				// Get the relevant sensor codes.
				List<String> sensorCodes = AirsEndpointRepository.getSensorCodes();

				// Display what we have subscribed to.
				if (sensorCodes != null) {
					StringBuilder builder = new StringBuilder(sensorCodes.size() * 3);

					// Subscribe to the relevant sensors.
					for (String sensorCode : sensorCodes) {
						this.m_Server.subscribe(sensorCode);
						builder.append(sensorCode).append(";");
					}
					this.m_Activity.setStatusText("subscribed to: " + builder.toString());
				}
			}

		} catch (IOException e) {

		}
	}

	private void storeComponentFiles() {
		// Create a FileBootloader to store our component file.
		new FileBootloader(this.m_Context).store("AirsSensorDynamic.cpt").store("AirsSensor.cpt");
	}

	private void createEndpoints() {
		// Create a RAM endpoint, add to component and to repository.
		AirsEndpoint ram = new AirsEndpoint(this.m_Component.addEndpoint("RAM", EndpointType.EndpointSource, "2AD6AEFD7646"), "Rm", TYPE.SInt, UIManager.getInstance().getUIHandler());
		AirsEndpointRepository.addEndpoint(ram);

		// Create a weather endpoint, add to component and to repository.
		AirsEndpoint weatherCondition = new AirsEndpoint(this.m_Component.addEndpoint("WeatherCondition", EndpointType.EndpointSource, "5726AEFD7346"), "VC", TYPE.SText, UIManager.getInstance().getUIHandler());
		AirsEndpointRepository.addEndpoint(weatherCondition);

		// Create a random number endpoint, add to component and to repository.
		AirsEndpoint randomNumber = new AirsEndpoint(this.m_Component.addEndpoint("Random", EndpointType.EndpointSource, "2AD6AEFD7646"), "Rd", TYPE.SInt, UIManager.getInstance().getUIHandler());
		AirsEndpointRepository.addEndpoint(randomNumber);
	}
}
