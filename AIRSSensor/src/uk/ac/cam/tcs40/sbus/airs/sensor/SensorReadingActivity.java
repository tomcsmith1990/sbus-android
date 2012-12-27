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
	private SComponent m_AirsComponent;

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
		this.m_AirsComponent = new SComponent("AirsSensor", "airs");

		// Create a RAM endpoint, add to component and to repository.
		final TextView ramTextView = (TextView) findViewById(R.id.ram);
		AirsEndpoint ram = new AirsEndpoint(this.m_AirsComponent.addEndpoint("RAM", "2AD6AE7D73C6"), "Rm", "ram", TYPE.SInt, new UIHandler(ramTextView));
		AirsEndpointRepository.addEndpoint(ram);
		
		// Create a weather endpoint, add to component and to repository.
		final TextView weatherTextView = (TextView) findViewById(R.id.weather); 
		AirsEndpoint weatherCondition = new AirsEndpoint(this.m_AirsComponent.addEndpoint("WeatherCondition", "07A6F46058A8"), "VC", "condition", TYPE.SText, new UIHandler(weatherTextView));
		AirsEndpointRepository.addEndpoint(weatherCondition);

		// Create a random number endpoint, add to component and to repository.
		final TextView randomTextView = (TextView) findViewById(R.id.random);
		AirsEndpoint randomNumber = new AirsEndpoint(this.m_AirsComponent.addEndpoint("Random", "CFAE86F7E614"), "Rd", "random", TYPE.SInt, new UIHandler(randomTextView));
		AirsEndpointRepository.addEndpoint(randomNumber);

		// Register RDC if it is available.
		this.m_AirsComponent.addRDC("192.168.0.3:50123");
		// 10.0.2.2 is the development machine when running in AVD.
		//scomponent.addRDC("10.0.2.2:50123");

		// Start the component, load the .cpt file.
		String cptFile = "AirsSensor.cpt";
		this.m_AirsComponent.start(getApplicationContext().getFilesDir() + "/" + cptFile, 44445, true);
		this.m_AirsComponent.setPermission("AirsConsumer", "", true);

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
}
