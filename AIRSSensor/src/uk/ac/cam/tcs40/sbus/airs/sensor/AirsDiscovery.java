package uk.ac.cam.tcs40.sbus.airs.sensor;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import android.util.Log;

import com.airs.platform.Callback;
import com.airs.platform.Discovery;
import com.airs.platform.EventComponent;
import com.airs.platform.SensorRepository;

public class AirsDiscovery extends Discovery implements Callback {

	private SensorReadingActivity m_Activity;
	private final String m_SensorFileName = "sensors.txt";
	private final Object m_SensorFileLock = new Object();

	public AirsDiscovery(EventComponent current_EC, SensorReadingActivity activity) {
		super(current_EC);
		this.m_Activity = activity;
	}

	public void callback(DIALOG_INFO dialog)
	{
		// Only care about PUBLISH, do nothing on other (if any) types.
		if (dialog.current_method.method_type == method_type.method_PUBLISH) {

			if (dialog.current_method.event_body.length > 0) {

				debug("Discovery::callback: received PUBLISH with at least one sensor");

				saveSensorsToFile(dialog.current_method.event_body.string);
				loadSensorsFromFile();
				
			} else {

				debug("Discovery::callback: received PUBLISH with no sensor information");
			}
		}

		// unlock dialog -> do not forget otherwise the dialog becomes unusuable!
		dialog.locked = false;
	}


	protected void parse(byte[] sensor_description, int length, int expires) {
		saveSensorsToFile(sensor_description);
		updateSensorList(sensor_description, length, expires);
	}

	private void updateSensorList(byte[] sensor_description, int length, int expires) {
		int offset = 0;
		int position = 0;

		// count lines first -> number of sensors
		while (position < length) {
			if (sensor_description[position] == '\r') {				

				String line = new String(sensor_description, offset, position - offset);

				// Skip over the \r
				offset = position + 1;

				// symbol::description::unit::type::scaler::min::max
				String[] params = line.split("::");

				SensorRepository.insertSensor(params[0], 
						params[2], 
						params[1], 
						params[3], 
						Integer.parseInt(params[4]), 
						Integer.parseInt(params[5]), 
						Integer.parseInt(params[6]),
						false, 30);
				
				this.m_Activity.addSensorToList(SensorRepository.findSensor(params[0]));

			}

			position++;
		}
	}

	public void loadSensorsFromFile() {
		synchronized (this.m_SensorFileLock) {

			File file = new File(this.m_Activity.getApplicationContext().getFilesDir(), this.m_SensorFileName);

			// Returns 0 if file does not exist.
			int length = (int) file.length();
			byte[] buffer = new byte[0];

			if (length > 0) {
				try {

					// Open an InputStream for the file.
					InputStream in = new FileInputStream(file);

					buffer = new byte[length];

					// Read the sensor descriptions from the file.
					in.read(buffer);

					// Close the file stream.
					in.close();

				} catch (IOException e) {
					Log.w("AirsDiscovery::callback", "Error reading sensor descriptions", e);
				}

				// Update sensor descriptions in the app.
				updateSensorList(buffer, length, 0);
			}
		}
	}

	private void saveSensorsToFile(byte[] sensorDescription) {
		synchronized (this.m_SensorFileLock) {

			File file = new File(this.m_Activity.getApplicationContext().getFilesDir(), this.m_SensorFileName);

			try {

				// Open an OutputStream for the file.
				OutputStream out = new FileOutputStream(file);

				// Write sensor descriptions to the file.
				out.write(sensorDescription);

				// Close the file streams.
				out.close();

			} catch (IOException e) {
				Log.w("AirsDiscovery::callback", "Error writing sensor descriptions", e);
			}
		}
	}

}
