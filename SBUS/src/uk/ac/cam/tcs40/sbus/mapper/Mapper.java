package uk.ac.cam.tcs40.sbus.mapper;

import java.util.LinkedList;
import java.util.List;

import uk.ac.cam.tcs40.sbus.FileBootloader;
import uk.ac.cam.tcs40.sbus.SComponent;
import uk.ac.cam.tcs40.sbus.SEndpoint;
import uk.ac.cam.tcs40.sbus.SMessage;
import uk.ac.cam.tcs40.sbus.SNode;
import uk.ac.cam.tcs40.sbus.sbus.R;
import android.app.Activity;
import android.content.IntentFilter;
import android.net.ConnectivityManager;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;


public class Mapper extends Activity
{
	private final int DEFAULT_RDC_PORT = 50123;
	private final String CPT_FILE = "connect.cpt";

	public static final String COMPONENT_ADDR = ":44444";
	public static final String COMPONENT_EPT = "SomeEpt";
	public static final String TARGET_ADDR = "192.168.0.3:44444";
	public static final String TARGET_EPT = "SomeEpt";
	public static final String RDC_ADDRESS = "192.168.0.3:50123";

	private SComponent m_ConnectComponent;
	private MapEndpoint m_MapEndpoint;
	private RdcEndpoint m_RdcEndpoint;
	private final List<String> m_Registered = new LinkedList<String>();

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
		// Display the layout.
		setContentView(R.layout.activity_mapper);

		// Add event listener to the map button.
		Button mapButton = (Button)findViewById(R.id.map_button);
		mapButton.setOnClickListener(m_MapButtonListener);

		// Add event listener to the rdc button.
		Button rdcButton = (Button)findViewById(R.id.rdc_button);
		rdcButton.setOnClickListener(m_RdcButtonListener);

		// Copy the needed SBUS files if they do not already exist.
		new SBUSBootloader(getApplicationContext());

		// Copy the .cpt file to the application directory.
		new FileBootloader(getApplicationContext()).store(CPT_FILE);

		doMapping();
	}

	private void doMapping() {
		// Our mapping/rdc component.
		this.m_ConnectComponent = new SComponent("rdc", "rdc");

		// For components registering to the rdc.
		final SEndpoint register_ep = this.m_ConnectComponent.addEndpointSink("register", "B3572388E4A4");

		// For components sending permissions after registering.
		final SEndpoint acl_ep = this.m_ConnectComponent.addEndpointSink("set_acl", "6AF2ED96750B");

		// For remapping components to other components.
		SEndpoint map = this.m_ConnectComponent.addEndpointSource("map", "F46B9113DB2D");
		this.m_MapEndpoint = new MapEndpoint(map);

		// For telling components to connect to an rdc.
		SEndpoint rdc = this.m_ConnectComponent.addEndpointSource("register_rdc", "3D3F1711E783");
		this.m_RdcEndpoint = new RdcEndpoint(rdc);

		// Start the component on the default rdc port.
		this.m_ConnectComponent.start(getApplicationContext().getFilesDir() + "/" + CPT_FILE, DEFAULT_RDC_PORT, false);

		// Allow all components to connect to endpoints (for register).
		this.m_ConnectComponent.setPermission("", "", true);

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

		new Thread() {
			public void run() {

				String address;
				boolean arrived;
				SMessage message;
				SNode snode;

				while (true) {
					message = register_ep.receive();
					snode = message.getTree();
					address = snode.extractString("address");
					arrived = snode.extractBoolean("arrived");

					if (arrived) {
						if (m_Registered.contains(address)) {
							Log.i("MPC", "Attempting to register already registered component.");
						} else {
							m_Registered.add(address);
							Log.i("MPC", "Registered component " + address);
						}
					} else {
						m_Registered.remove(address);
						Log.i("MPC", "Deregistered component " + address);
					}

					message.delete();
				}

			}
		}.start();
	}
}