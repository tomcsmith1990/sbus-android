package uk.ac.cam.tcs40.sbus.airs.sensor;

import java.io.IOException;
import java.util.LinkedList;
import java.util.List;

import com.airs.platform.Acquisition;
import com.airs.platform.EventComponent;
import com.airs.platform.Server;

import uk.ac.cam.tcs40.sbus.FileBootloader;
import uk.ac.cam.tcs40.sbus.SComponent;
import uk.ac.cam.tcs40.sbus.airs.sensor.AirsEndpoint.TYPE;
import uk.ac.cam.tcs40.sbus.airs.sensor.dynamic.EndpointManager;
import uk.ac.cam.tcs40.sbus.airs.sensor.dynamic.UIManager;
import android.os.Bundle;
import android.app.Activity;
import android.widget.TextView;

public class SensorReadingActivity extends Activity {

	private SensorReadingDB m_AirsDb;
	private TextView m_StatusTextView;
	private SComponent m_AirsComponent;

	/** Called when the activity is first created. */
	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_airs);
		
		// Set this as the activity so we can create TextViews.
		UIManager.getInstance(this);

		this.m_StatusTextView = (TextView) findViewById(R.id.status);

		/*** CONFIGURATION ***/
		final boolean liveReadings = true;
		final boolean dynamicEndpoints = true;

		// Store the component files used.
		storeComponentFiles();

		// Create the component.
		this.m_AirsComponent = new SComponent("AirsSensor", "airs");

		String cptFile;

		// The component file to use.
		if (dynamicEndpoints) {
			cptFile = "AirsSensorDynamic.cpt";
		} else {
			cptFile = "AirsSensor.cpt";
			createEndpoints();
		}

		// Register RDC if it is available.
		this.m_AirsComponent.addRDC("192.168.0.3:50123");
		// 10.0.2.2 is the development machine when running in AVD.
		//scomponent.addRDC("10.0.2.2:50123");

		// Start the component, load the .cpt file.
		this.m_AirsComponent.start(getApplicationContext().getFilesDir() + "/" + cptFile, 44445, true);
		this.m_AirsComponent.setPermission("AirsConsumer", "", true);

		if (liveReadings) {

			new Thread() {
				@Override
				public void run() {
					
					// Set up the stuff for the AIRS server.
					EventComponent eventComponent = new EventComponent();
					Acquisition acquisition;

					if (dynamicEndpoints) {
						EndpointManager endpointManager = new EndpointManager(getApplicationContext(), m_AirsComponent);
						acquisition = new AirsAcquisition(eventComponent, endpointManager);
					} else {
						acquisition = new AirsAcquisition(eventComponent, null);
					}

					Server server = new Server(9000, eventComponent, acquisition);
					try {
						setStatusText("waiting for AIRS to connect");						
						// Start a server waiting for AIRS Remote to connect.
						server.startConnection();

						setStatusText("subscribing to AIRS sensors");

						List<String> sensorCodes;

						// Get the sensors to subscribe to.
						if (dynamicEndpoints) {
							sensorCodes = new LinkedList<String>();
							sensorCodes.add("Rm");
							sensorCodes.add("VC");
							sensorCodes.add("Rd");
						} else {
							// Get the relevant sensor codes.
							sensorCodes = AirsEndpointRepository.getSensorCodes();
						}

						// Display what we have subscribed to.
						if (sensorCodes != null) {
							StringBuilder builder = new StringBuilder(sensorCodes.size() * 3);

							// Subscribe to the relevant sensors.
							for (String sensorCode : sensorCodes) {
								server.subscribe(sensorCode);
								builder.append(sensorCode).append(";");
							}
							setStatusText("subscribed to: " + builder.toString());
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

	@Override
	protected void onDestroy() {
		this.m_AirsComponent.delete();
		super.onDestroy();
	}

	public void setStatusText(final String message) {
		runOnUiThread(new Runnable() {
			public void run() {
				m_StatusTextView.setText(message);
			}
		});
	}

	private void storeComponentFiles() {
		// Create a FileBootloader to store our component file.
		new FileBootloader(getApplicationContext()).store("AirsSensorDynamic.cpt").store("AirsSensor.cpt");
	}

	private void createEndpoints() {
		// Create a RAM endpoint, add to component and to repository.
		AirsEndpoint ram = new AirsEndpoint(this.m_AirsComponent.addEndpointSource("RAM", "2AD6AEFD7646"), "Rm", TYPE.SInt, UIManager.getInstance().getUIHandler());
		AirsEndpointRepository.addEndpoint(ram);

		// Create a weather endpoint, add to component and to repository.
		AirsEndpoint weatherCondition = new AirsEndpoint(this.m_AirsComponent.addEndpointSource("WeatherCondition", "5726AEFD7346"), "VC", TYPE.SText, UIManager.getInstance().getUIHandler());
		AirsEndpointRepository.addEndpoint(weatherCondition);

		// Create a random number endpoint, add to component and to repository.
		AirsEndpoint randomNumber = new AirsEndpoint(this.m_AirsComponent.addEndpointSource("Random", "2AD6AEFD7646"), "Rd", TYPE.SInt, UIManager.getInstance().getUIHandler());
		AirsEndpointRepository.addEndpoint(randomNumber);
	}
}
