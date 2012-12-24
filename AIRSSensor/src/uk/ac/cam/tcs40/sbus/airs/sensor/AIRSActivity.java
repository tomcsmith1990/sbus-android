package uk.ac.cam.tcs40.sbus.airs.sensor;

import java.io.IOException;

import com.airs.platform.Acquisition;
import com.airs.platform.EventComponent;
import com.airs.platform.Server;

import uk.ac.cam.tcs40.sbus.FileBootloader;
import uk.ac.cam.tcs40.sbus.SComponent;
import uk.ac.cam.tcs40.sbus.airs.sensor.AirsEndpoint.TYPE;
import android.os.Bundle;
import android.app.Activity;
import android.widget.TextView;

public class AIRSActivity extends Activity {

	private AirsDb m_AirsDb;

	/** Called when the activity is first created. */
	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_airs);

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
		//AirsEndpointRepository.addEndpoint(weatherCondition);
		component.addEndpoint(weatherCondition);

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
	
			EventComponent eventComponent = new EventComponent();
			Acquisition acquisition = new AirsAcquisition(eventComponent);
			
			Server server = new Server(9000, eventComponent, acquisition);
			try {
				// Start a server waiting for AIRS Remote to connect.
				server.startConnection();
				// Subscribe to the relevant sensors.
				for (String sensorCode : AirsEndpointRepository.getSensorCodes())
					server.subscribe(sensorCode);

			} catch (IOException e) {

			}
			
		} else {
			// Create threads for relevant sensors.
			for (String sensorCode : AirsEndpointRepository.getSensorCodes())
				new Thread(new EndpointThread(m_AirsDb, sensorCode)).start();
		}
	}
}
