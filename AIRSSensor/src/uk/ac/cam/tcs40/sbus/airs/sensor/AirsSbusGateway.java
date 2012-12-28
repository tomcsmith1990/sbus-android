package uk.ac.cam.tcs40.sbus.airs.sensor;

import java.io.IOException;
import java.util.LinkedList;
import java.util.List;

import com.airs.platform.Acquisition;
import com.airs.platform.EventComponent;
import com.airs.platform.Server;

import android.content.Context;
import uk.ac.cam.tcs40.sbus.FileBootloader;
import uk.ac.cam.tcs40.sbus.SComponent;
import uk.ac.cam.tcs40.sbus.airs.sensor.AirsEndpoint.TYPE;
import uk.ac.cam.tcs40.sbus.airs.sensor.dynamic.EndpointManager;
import uk.ac.cam.tcs40.sbus.airs.sensor.dynamic.UIManager;

public class AirsSbusGateway {

	/*** CONFIGURATION ***/
	final boolean m_LiveReadings = true;
	final boolean m_DynamicEndpoints = true;
	
	private SComponent m_Component;
	private SensorReadingActivity m_Activity;
	private Context m_Context;
	private Server m_Server;
	private SensorReadingDB m_AirsDb;
	
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
		// 10.0.2.2 is the development machine when running in AVD.
		//scomponent.addRDC("10.0.2.2:50123");

		// Start the component, load the .cpt file.
		this.m_Component.start(this.m_Context.getFilesDir() + "/" + cptFile, 44445, true);
		this.m_Component.setPermission("AirsConsumer", "", true);
	}
	
	public void startReadings() {
		if (this.m_LiveReadings) {

			new Thread() {
				@Override
				public void run() {

					// Set up the stuff for the AIRS server.
					EventComponent eventComponent = new EventComponent();
					Acquisition acquisition;

					if (m_DynamicEndpoints) {
						EndpointManager endpointManager = new EndpointManager(m_Context, m_Component);
						acquisition = new AirsAcquisition(eventComponent, endpointManager);
					} else {
						acquisition = new AirsAcquisition(eventComponent, null);
					}

					m_Server = new Server(9000, eventComponent, acquisition);
					
					try {
						m_Activity.setStatusText("waiting for AIRS to connect");						
						// Start a server waiting for AIRS Remote to connect.
						m_Server.startConnection();

						m_Activity.setStatusText("subscribing to AIRS sensors");

						List<String> sensorCodes;

						// Get the sensors to subscribe to.
						if (m_DynamicEndpoints) {
							sensorCodes = new LinkedList<String>();
							//sensorCodes.add("Rm");
							//sensorCodes.add("VC");
							//sensorCodes.add("Rd");
						} else {
							// Get the relevant sensor codes.
							sensorCodes = AirsEndpointRepository.getSensorCodes();
						}

						// Display what we have subscribed to.
						if (sensorCodes != null) {
							StringBuilder builder = new StringBuilder(sensorCodes.size() * 3);

							// Subscribe to the relevant sensors.
							for (String sensorCode : sensorCodes) {
								m_Server.subscribe(sensorCode);
								builder.append(sensorCode).append(";");
							}
							m_Activity.setStatusText("subscribed to: " + builder.toString());
						}

					} catch (IOException e) {

					}
				}
			}.start();

		} else {
			// Create threads for relevant sensors.
			for (String sensorCode : AirsEndpointRepository.getSensorCodes())
				new Thread(new DBReadingHandler(m_AirsDb, sensorCode)).start();
		}
	}
	
	public void subscribe(String sensorCode) {
		this.m_Server.subscribe(sensorCode);
	}
	
	public void stop() {
		this.m_Component.delete();
	}
	
	private void storeComponentFiles() {
		// Create a FileBootloader to store our component file.
		new FileBootloader(this.m_Context).store("AirsSensorDynamic.cpt").store("AirsSensor.cpt");
	}

	private void createEndpoints() {
		// Create a RAM endpoint, add to component and to repository.
		AirsEndpoint ram = new AirsEndpoint(this.m_Component.addEndpointSource("RAM", "2AD6AEFD7646"), "Rm", TYPE.SInt, UIManager.getInstance().getUIHandler());
		AirsEndpointRepository.addEndpoint(ram);

		// Create a weather endpoint, add to component and to repository.
		AirsEndpoint weatherCondition = new AirsEndpoint(this.m_Component.addEndpointSource("WeatherCondition", "5726AEFD7346"), "VC", TYPE.SText, UIManager.getInstance().getUIHandler());
		AirsEndpointRepository.addEndpoint(weatherCondition);

		// Create a random number endpoint, add to component and to repository.
		AirsEndpoint randomNumber = new AirsEndpoint(this.m_Component.addEndpointSource("Random", "2AD6AEFD7646"), "Rd", TYPE.SInt, UIManager.getInstance().getUIHandler());
		AirsEndpointRepository.addEndpoint(randomNumber);
	}
}
