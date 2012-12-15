package uk.ac.cam.tcs40.sbus;

import uk.ac.cam.tcs40.sbus.sbus.R;
import android.app.Activity;
import android.content.IntentFilter;
import android.net.ConnectivityManager;
import android.net.wifi.WifiManager;
import android.os.Bundle;


public class Mapper extends Activity
{
	/** Called when the activity is first created. */
	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		
		setContentView(R.layout.activity_mapper);

		// Create the SBUSBootloader.
		// This will copy all necessary SBUS files.
		new SBUSBootloader(getApplicationContext());

		new FileBootloader(getApplicationContext()).store("map.cpt");

		// Create a thread to run the sensor.
		/*new Thread() {
			public void run() {
			  	// Create a FileBootloader to store our component file.
  				new FileBootloader(getApplicationContext()).store("SomeSensor.cpt");

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
		}.start();*/
		/*
		new Thread() {
			public void run() {				
				SComponent mapComponent = new SComponent("spoke", "spoke");
				mapComponent.addEndpoint("map", "F46B9113DB2D");
				String mapFile = "map.cpt";
				mapComponent.start(getApplicationContext().getFilesDir() + "/" + mapFile,  -1, false);

				mapComponent.endpointMap(":44444");

				mapComponent.createMessage("map");
				mapComponent.packString("SomeEpt", "endpoint");
				mapComponent.packString("192.168.0.3:44444", "peer_address");
				mapComponent.packString("SomeEpt", "peer_endpoint");
				mapComponent.packString("", "certificate");

				final String s = mapComponent.emit();

				runOnUiThread(new Runnable() {
					public void run() {
						tv.setText(s);
					}
				});
			}
		}.start();
		 */	
		new Thread() {
			public void run() {
				MapComponent mapComponent = new MapComponent("spoke", "spoke");
				mapComponent.addEndpoint("map", "F46B9113DB2D");
				String mapFile = "map.cpt";
				mapComponent.start(getApplicationContext().getFilesDir() + "/" + mapFile,  -1, false);

				IntentFilter intentFilter = new IntentFilter();
				intentFilter.addAction(ConnectivityManager.CONNECTIVITY_ACTION);
				intentFilter.addAction(WifiManager.NETWORK_STATE_CHANGED_ACTION);
				registerReceiver(new WifiReceiver(mapComponent), intentFilter);

				while(true) {
					try {
						Thread.sleep(1000);
					} catch (InterruptedException e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}
				}
			}
		}.start();
	}    

}
