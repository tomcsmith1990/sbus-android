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
	private SComponent m_ConnectComponent;
	private MapEndpoint m_MapEndpoint;
	private RdcEndpoint m_RdcEndpoint;
	
	public static final String COMPONENT_ADDR = ":44444";
	public static final String COMPONENT_EPT = "Rd";
	public static final String TARGET_ADDR = "192.168.0.3:44444";
	public static final String TARGET_EPT = "Random";
	public static final String RDC_ADDRESS = "128.232.128.128:50123";

	private OnClickListener m_MapButtonListener = new OnClickListener() {
		public void onClick(View v) {
			remap(m_MapEndpoint);
		}
	};
	
	private OnClickListener m_RdcButtonListener = new OnClickListener() {
		public void onClick(View v) {
			registerRdc(m_RdcEndpoint);
		}
	};
	
	public static void remap(MapEndpoint endpoint) {
		endpoint.map(Mapper.COMPONENT_ADDR, Mapper.COMPONENT_EPT, Mapper.TARGET_ADDR, Mapper.TARGET_EPT);
	}
	
	public static void registerRdc(RdcEndpoint endpoint) {
		endpoint.registerRdc(Mapper.COMPONENT_ADDR, Mapper.RDC_ADDRESS);
	}

	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);

		// Copy the needed SBUS files if they do not already exist.
		new SBUSBootloader(getApplicationContext());

		// .cpt file for this component.
		final String mapFile = "connect.cpt";

		// Copy the .cpt file to the application directory.
		new FileBootloader(getApplicationContext()).store(mapFile);

		// Initialise and start the map component.
		this.m_ConnectComponent = new SComponent("spoke", "spoke");
		SEndpoint map = this.m_ConnectComponent.addEndpointSource("map", "F46B9113DB2D");
		this.m_MapEndpoint = new MapEndpoint(map);
		
		SEndpoint rdc = this.m_ConnectComponent.addEndpointSource("emit", "3D3F1711E783");
		this.m_RdcEndpoint = new RdcEndpoint(rdc);
		
		this.m_ConnectComponent.start(getApplicationContext().getFilesDir() + "/" + mapFile, -1, false);

		// Display the layout.
		setContentView(R.layout.activity_mapper);

		// Add event listener to the map button.
		Button mapButton = (Button)findViewById(R.id.map_button);
		mapButton.setOnClickListener(m_MapButtonListener);
		
		// Add event listener to the rdc button.
		Button rdcButton = (Button)findViewById(R.id.rdc_button);
		rdcButton.setOnClickListener(m_RdcButtonListener);

		new Thread() {
			public void run() {
				// Register a broadcast receiver.
				// Detects when we connect/disconnect to a Wifi network.
				IntentFilter intentFilter = new IntentFilter();
				intentFilter.addAction(ConnectivityManager.CONNECTIVITY_ACTION);
				intentFilter.addAction(WifiManager.NETWORK_STATE_CHANGED_ACTION);
				registerReceiver(new WifiReceiver(m_MapEndpoint, m_RdcEndpoint), intentFilter);

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