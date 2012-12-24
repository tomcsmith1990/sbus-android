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

		final TextView batteryTextView = (TextView) findViewById(R.id.battery);
		final TextView weatherTextView = (TextView) findViewById(R.id.weather); 

		SComponent component = new SComponent("AirsSensor", "airs");

		AirsEndpoint batteryVoltage = new AirsEndpoint("BatteryVoltage", "6F7AA0BB0B8F", "BV", "batteryVoltage", TYPE.SInt, new UIHandler(batteryTextView));
		component.addEndpoint(batteryVoltage);

		AirsEndpoint weatherCondition = new AirsEndpoint("WeatherCondition", "3D390E79C4A8", "VC", "condition", TYPE.SText, new UIHandler(weatherTextView));
		component.addEndpoint(weatherCondition);

		component.addRDC("192.168.0.3:50123");
		// 10.0.2.2 is the development machine when running in AVD.
		//scomponent.addRDC("10.0.2.2:50123");
		String cptFile = "AirsSensor.cpt";
		component.start(getApplicationContext().getFilesDir() + "/" + cptFile, 44445, true);
		component.setPermission("AirsConsumer", "", true);

		boolean liveReadings = true;

		if (liveReadings) {
			AirsEndpointRepository.addEndpoint(batteryVoltage);
			AirsEndpointRepository.addEndpoint(weatherCondition);
			
			EventComponent eventComponent = new EventComponent();
			Acquisition acquisition = new AirsAcquisition(eventComponent);
			
			Server server = new Server(9000, eventComponent, acquisition);
			try {
				server.startConnection();
				server.subscribe(batteryVoltage.getSensorCode());
				server.subscribe(weatherCondition.getSensorCode());
			} catch (IOException e) {

			}

		} else {

			// Create a thread to run the sensor.
			new Thread(new EndpointThread(m_AirsDb, batteryVoltage)).start();
			new Thread(new EndpointThread(m_AirsDb, weatherCondition)).start();
		}
	}
}
