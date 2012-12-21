package uk.ac.cam.tcs40.sbus.airs.sensor;

import uk.ac.cam.tcs40.sbus.FileBootloader;
import uk.ac.cam.tcs40.sbus.SComponent;
import uk.ac.cam.tcs40.sbus.SEndpoint;
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

		// Add a TextView to the Activity.
		final TextView tv = new TextView(this);
		tv.setText("AIRS Sensor");
		setContentView(tv);

		UIHandler uiHandler = new UIHandler(tv);
		
		SComponent component = new SComponent("AirsSensor", "airs");
		
		SEndpoint batteryVoltage = new SEndpoint("BatteryVoltage", "6F7AA0BB0B8F");
		component.addEndpoint(batteryVoltage);
		
		component.addRDC("192.168.0.3:50123");
		// 10.0.2.2 is the development machine when running in AVD.
		//scomponent.addRDC("10.0.2.2:50123");
		String cptFile = "AirsSensor.cpt";
		component.start(getApplicationContext().getFilesDir() + "/" + cptFile, 44445, true);
		component.setPermission("AirsConsumer", "", true);
		
		// Create a thread to run the sensor.
		new Thread(new EndpointThread(m_AirsDb, uiHandler, batteryVoltage)).start();
	}
}
