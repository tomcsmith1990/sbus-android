package uk.ac.cam.tcs40.sbus.airs.sensor;

import java.util.Date;

import uk.ac.cam.tcs40.sbus.FileBootloader;
import uk.ac.cam.tcs40.sbus.SComponent;
import uk.ac.cam.tcs40.sbus.SEndpoint;
import android.os.Bundle;
import android.app.Activity;
import android.database.Cursor;
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
		new FileBootloader(getApplicationContext()).store("SomeSensor.cpt");

		// Add a TextView to the Activity.
		final TextView tv = new TextView(this);
		tv.setText("AIRS Sensor");
		setContentView(tv);

		// Create a thread to run the sensor.
		new Thread() {
			public void run() {
				SComponent scomponent = new SComponent("SomeSensor", "airs");
				SEndpoint batteryVoltageEndpoint = new SEndpoint("BatteryVoltage", "6F7AA0BB0B8F");
				scomponent.addEndpoint(batteryVoltageEndpoint);
				scomponent.addRDC("192.168.0.3:50123");
				// 10.0.2.2 is the development machine when running in AVD.
				//scomponent.addRDC("10.0.2.2:50123");
				String cptFile = "SomeSensor.cpt";
				scomponent.start(getApplicationContext().getFilesDir() + "/" + cptFile, 44445, true);
				scomponent.setPermission("SomeConsumer", "", true);

				int i = 0;

				// Open database and query for values.
				m_AirsDb = new AirsDb();
				m_AirsDb.open();
				Cursor values = m_AirsDb.query();

				// Recrods from database.
				int batteryVoltage = -1;
				long timestamp = 0;

				// Get column indexes.
				int timeColumn = values.getColumnIndex("Timestamp");
				int valueColumn = values.getColumnIndex("Value");

				boolean haveReadings = false;

				// If there are records, read the first one.
				if (values.moveToFirst()) {
					batteryVoltage = Integer.valueOf(values.getString(valueColumn));
					timestamp = values.getLong(timeColumn);
					haveReadings = true;
				}

				while (true) {

					batteryVoltageEndpoint.createMessage("reading");
					batteryVoltageEndpoint.packString("AIRS: This is message #" + i++);
					batteryVoltageEndpoint.packTime(new Date(timestamp), "timestamp");
					batteryVoltageEndpoint.packInt(batteryVoltage, "batteryVoltage");

					final String s = batteryVoltageEndpoint.emit();

					runOnUiThread(new Runnable() {
						public void run() {
							tv.setText(s);
						}
					});

					// Move to and read next record if possible.
					// If not, set to some default values and close database.
					if (haveReadings && values.moveToNext()) {
						batteryVoltage = Integer.valueOf(values.getString(valueColumn));
						timestamp = values.getLong(timeColumn);
					} else {
						haveReadings = false;
						m_AirsDb.close();
						values = null;
						batteryVoltage = -1;
						timestamp = 0;
					}

					try {
						Thread.sleep(2000);
					} catch (InterruptedException e) { }
				}
			}
		}.start();

	}
}
