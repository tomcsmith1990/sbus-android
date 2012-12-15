package uk.ac.cam.tcs40.sbus;

import uk.ac.cam.tcs40.sbus.sbus.R;
import android.app.Activity;
import android.content.IntentFilter;
import android.net.ConnectivityManager;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;


public class Mapper extends Activity
{
	private MapComponent m_MapComponent;

	private OnClickListener m_MapButtonListener = new OnClickListener() {
		public void onClick(View v) {
			m_MapComponent.map(":44444", "SomeEpt", "192.168.0.6:44444", "SomeEpt");
		}
	};

	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);

		// Create the SBUSBootloader.
		// This will copy all necessary SBUS files.
		new SBUSBootloader(getApplicationContext());

		final String mapFile = "map.cpt";
		
		new FileBootloader(getApplicationContext()).store(mapFile);

		this.m_MapComponent = new MapComponent("spoke", "spoke");
		this.m_MapComponent.addEndpoint("map", "F46B9113DB2D");
		this.m_MapComponent.start(getApplicationContext().getFilesDir() + "/" + mapFile,  -1, false);
	
		setContentView(R.layout.activity_mapper);
		
		Button button = (Button)findViewById(R.id.map_button);
		button.setOnClickListener(m_MapButtonListener);

		new Thread() {
			public void run() {
				IntentFilter intentFilter = new IntentFilter();
				intentFilter.addAction(ConnectivityManager.CONNECTIVITY_ACTION);
				intentFilter.addAction(WifiManager.NETWORK_STATE_CHANGED_ACTION);
				registerReceiver(new WifiReceiver(m_MapComponent), intentFilter);

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

		// Create a thread to run the sensor.
		/*new Thread() {
			public 		setContentView(R.layout.activity_mapper);void run() {
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
	}
}