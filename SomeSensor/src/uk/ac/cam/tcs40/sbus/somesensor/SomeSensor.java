package uk.ac.cam.tcs40.sbus.somesensor;

import uk.ac.cam.tcs40.sbus.SComponent;
import uk.ac.cam.tcs40.sbus.SComponent.EndpointType;
import uk.ac.cam.tcs40.sbus.SEndpoint;
import uk.ac.cam.tcs40.sbus.FileBootloader;
import uk.ac.cam.tcs40.sbus.SNode;
import android.app.Activity;
import android.widget.TextView;
import android.os.Bundle;

public class SomeSensor extends Activity
{
	/** Called when the activity is first created. */
	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);

		// Create a FileBootloader to store our component file.
  		new FileBootloader(getApplicationContext()).store("SomeSensor.cpt");

  		// Add a TextView to the Activity.
		final TextView tv = new TextView(this);
		tv.setText("Some Sensor");
		setContentView(tv);
		
		// Create a thread to run the sensor.
		new Thread() {
			public void run() {
				SComponent scomponent = new SComponent("SomeSensor", "instance");
				SEndpoint sendpoint = scomponent.addEndpoint("SomeEpt", EndpointType.EndpointSource, "BE8A47EBEB58");
				// 10.0.2.2 is the development machine when running in AVD.
				//scomponent.addRDC("10.0.2.2:50123");
				String cptFile = "SomeSensor.cpt";
				scomponent.setRDCUpdateAutoconnect(true);
				scomponent.start(getApplicationContext().getFilesDir() + "/" + cptFile, 44444, true);
				scomponent.setPermission("SomeConsumer", "", true);
			
				int i = 0;
				SNode node;
				while (true) {

					node = sendpoint.createMessage("reading");
					node.packString("SomeSensor: This is message #" + i++);
					node.packInt((int) (Math.random() * 1000), "someval");
					node.packInt(34, "somevar");
					
					final String s = sendpoint.emit(node);

					runOnUiThread(new Runnable() {
						public void run() {
							tv.setText(s);
						}
					});

					try {
						Thread.sleep(2000);
					} catch (InterruptedException e) { }
				}
			}
		}.start();

	}   
}