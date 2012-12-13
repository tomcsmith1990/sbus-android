package uk.ac.cam.tcs40.sbus;

import android.app.Activity;
import android.widget.TextView;
import android.os.Bundle;


public class Mapper extends Activity
{
	/** Called when the activity is first created. */
	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		
		// Create the SBUSBootloader.
		// This will copy all necessary SBUS files.
  		new SBUSBootloader(getApplicationContext());
  		
  		// Create a FileBootloader to store our component file.
  		new FileBootloader(getApplicationContext()).store("SomeSensor.cpt");;

  		// Add a TextView to the Activity.
		final TextView tv = new TextView(this);
		tv.setText("SBUS Mapper");
		setContentView(tv);
		
		// Create a thread to run the sensor.
		new Thread() {
			public void run() {
				SComponent scomponent = new SComponent("SomeSensor", "sensor-instance");
				scomponent.addEndpoint("SomeEpt", "BE8A47EBEB58");
				scomponent.addRDC("192.168.0.3:50123");
				// 10.0.2.2 is the development machine when running in AVD.
				//scomponent.addRDC("10.0.2.2:50123");
				String cptFile = "SomeSensor.cpt";
				scomponent.start(getApplicationContext().getFilesDir() + "/" + cptFile, -1, true);
				scomponent.setPermission("SomeConsumer", "", true);
			
				int i = 0;
				while (true) {

					scomponent.createMessage("reading");
					scomponent.packString("This is message #" + i++);
					scomponent.packInt((int) (Math.random() * 1000), "someval");
					scomponent.packInt(5, "somevar");
					
					final String s = scomponent.emit();

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
/*
		new Thread() {
			public void run() {
				new FileBootloader(getApplicationContext()).store("map.cpt");
				
				SComponent mapComponent = new SComponent("map", "map_instance");
				mapComponent.addEndpoint("map", "F46B9113DB2D");
				String mapFile = "map.cpt";
				mapComponent.start(mapFile,  -1, false);
				
				mapComponent.createMessage("map");
				mapComponent.packString("", "certificate");
				mapComponent.packString("SomeEpt", "endpoint");
				mapComponent.packString(":44444", "peer_address");
				mapComponent.packString("SomeEpt", "peer_endpoint");
			}
		};
*/	}    

}
