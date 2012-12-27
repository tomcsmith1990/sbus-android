package uk.ac.cam.tcs40.sbus.mapper;

import uk.ac.cam.tcs40.sbus.FileBootloader;
import uk.ac.cam.tcs40.sbus.SComponent;
import uk.ac.cam.tcs40.sbus.SEndpoint;
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
	private SComponent m_MapComponent;
	private MapEndpoint m_MapEndpoint;

	private OnClickListener m_MapButtonListener = new OnClickListener() {
		public void onClick(View v) {
			remap(m_MapEndpoint);
		}
	};
	
	public static void remap(MapEndpoint endpoint) {
		endpoint.map(":44445", "Random", "192.168.0.3:44444", "Random");
	}

	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);

		// Copy the needed SBUS files if they do not already exist.
		new SBUSBootloader(getApplicationContext());

		// .cpt file for this component.
		final String mapFile = "map.cpt";

		// Copy the .cpt file to the application directory.
		new FileBootloader(getApplicationContext()).store(mapFile);

		// Initialise and start the map component.
		this.m_MapComponent = new SComponent("spoke", "spoke");
		SEndpoint ept = this.m_MapComponent.addEndpointSource("map", "F46B9113DB2D");
		this.m_MapEndpoint = new MapEndpoint(ept);
		this.m_MapComponent.start(getApplicationContext().getFilesDir() + "/" + mapFile,  -1, false);

		// Display the layout.
		setContentView(R.layout.activity_mapper);

		// Add event listener to the map button.
		Button button = (Button)findViewById(R.id.map_button);
		button.setOnClickListener(m_MapButtonListener);

		new Thread() {
			public void run() {
				// Register a broadcast receiver.
				// Detects when we connect/disconnect to a Wifi network.
				IntentFilter intentFilter = new IntentFilter();
				intentFilter.addAction(ConnectivityManager.CONNECTIVITY_ACTION);
				intentFilter.addAction(WifiManager.NETWORK_STATE_CHANGED_ACTION);
				registerReceiver(new WifiReceiver(m_MapEndpoint), intentFilter);

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