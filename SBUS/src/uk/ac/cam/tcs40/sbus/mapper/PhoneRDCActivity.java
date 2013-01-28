package uk.ac.cam.tcs40.sbus.mapper;

import uk.ac.cam.tcs40.sbus.FileBootloader;
import uk.ac.cam.tcs40.sbus.sbus.R;
import android.app.Activity;
import android.content.IntentFilter;
import android.net.ConnectivityManager;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;


public class PhoneRDCActivity extends Activity
{
	private PhoneRDC m_PhoneRDC;
	
	private OnClickListener m_MapButtonListener = new OnClickListener() {
		public void onClick(View v) {
			PhoneRDC.remap();
		}
	};

	private OnClickListener m_RdcButtonListener = new OnClickListener() {
		public void onClick(View v) {
			PhoneRDC.registerRDC(true);
		}
	};

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
				
		new Thread() {
			public void run() {
				
				// Copy the needed SBUS files if they do not already exist.
				new SBUSBootloader(getApplicationContext());

				// Copy the .cpt file to the application directory.
				new FileBootloader(getApplicationContext()).store(PhoneRDC.CPT_FILE);

				// Create and start the PhoneRDC.
				m_PhoneRDC = new PhoneRDC(getApplicationContext());
				m_PhoneRDC.startRDC();

				// Register a broadcast receiver.
				// Detects when we connect/disconnect to a Wifi network.
				IntentFilter intentFilter = new IntentFilter();
				intentFilter.addAction(ConnectivityManager.CONNECTIVITY_ACTION);
				intentFilter.addAction(WifiManager.NETWORK_STATE_CHANGED_ACTION);
				registerReceiver(new WifiReceiver(), intentFilter);
			}
		}.start();
	}
	
	@Override
	public void onDestroy() {
		super.onDestroy();
		m_PhoneRDC.stopRDC();
	}
}