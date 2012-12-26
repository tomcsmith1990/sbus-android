package uk.ac.cam.tcs40.sbus.airs.sensor;

import java.io.IOException;
import java.util.List;

import com.airs.platform.Acquisition;
import com.airs.platform.EventComponent;
import com.airs.platform.Server;

import uk.ac.cam.tcs40.sbus.FileBootloader;
import uk.ac.cam.tcs40.sbus.SComponent;
import uk.ac.cam.tcs40.sbus.airs.sensor.AirsEndpoint.TYPE;
import android.os.Bundle;
import android.app.Activity;
import android.widget.TextView;

public class SensorReadingActivity extends Activity {

	private SensorReadingDB m_AirsDb;
	private TextView m_StatusTextView;

	/** Called when the activity is first created. */
	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_airs);
		this.m_StatusTextView = (TextView) findViewById(R.id.status);
		
		// Create a FileBootloader to store our component file.
		new FileBootloader(getApplicationContext()).store("AirsSensor.cpt");

		// Create the component.
		SComponent component = new SComponent("AirsSensor", "airs");

		// Create a battery voltage endpoint, add to component and to repository.
		final TextView batteryTextView = (TextView) findViewById(R.id.battery);
		AirsEndpoint batteryVoltage = new AirsEndpoint("BatteryVoltage", "6F7AA0BB0B8F", "BV", "batteryVoltage", TYPE.SInt, new UIHandler(batteryTextView));
		AirsEndpointRepository.addEndpoint(batteryVoltage);
		component.addEndpoint(batteryVoltage);

		// Create a weather endpoint, add to component and to repository.
		final TextView weatherTextView = (TextView) findViewById(R.id.weather); 
		AirsEndpoint weatherCondition = new AirsEndpoint("WeatherCondition", "3D390E79C4A8", "VC", "condition", TYPE.SText, new UIHandler(weatherTextView));
		AirsEndpointRepository.addEndpoint(weatherCondition);
		component.addEndpoint(weatherCondition);

		// Create a random number endpoint, add to component and to repository.
		final TextView randomTextView = (TextView) findViewById(R.id.random);
		AirsEndpoint randomNumber = new AirsEndpoint("Random", "DBCE88A476E2", "Rd", "random", TYPE.SInt, new UIHandler(randomTextView));
		AirsEndpointRepository.addEndpoint(randomNumber);
		component.addEndpoint(randomNumber);

		// Register RDC if it is available.
		component.addRDC("192.168.0.3:50123");
		// 10.0.2.2 is the development machine when running in AVD.
		//scomponent.addRDC("10.0.2.2:50123");

		// Start the component, load the .cpt file.
		String cptFile = "AirsSensor.cpt";
		component.start(getApplicationContext().getFilesDir() + "/" + cptFile, 44445, true);
		component.setPermission("AirsConsumer", "", true);

		boolean liveReadings = true;

		if (liveReadings) {

			new Thread() {
				@Override
				public void run() {
					
					EventComponent eventComponent = new EventComponent();
					Acquisition acquisition = new AirsAcquisition(eventComponent);

					Server server = new Server(9000, eventComponent, acquisition);
					try {
						setStatusText("waiting for AIRS to connect");						
						// Start a server waiting for AIRS Remote to connect.
						server.startConnection();
						
						setStatusText("subscribing to AIRS sensors");
						
						// Get the relevant sensor codes.
						List<String> sensorCodes = AirsEndpointRepository.getSensorCodes();
						
						StringBuilder builder = new StringBuilder(sensorCodes.size() * 3);
						
						// Subscribe to the relevant sensors.
						for (String sensorCode : sensorCodes) {
							server.subscribe(sensorCode);
							builder.append(sensorCode).append(";");
						}
						
						setStatusText("subscribed to: " + builder.toString());
							
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
	
	public void setStatusText(final String message) {
		runOnUiThread(new Runnable() {
			public void run() {
				m_StatusTextView.setText(message);
			}
		});
	}
}
