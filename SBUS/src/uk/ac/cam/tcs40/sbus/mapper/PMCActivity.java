package uk.ac.cam.tcs40.sbus.mapper;

import uk.ac.cam.tcs40.sbus.FileBootloader;
import uk.ac.cam.tcs40.sbus.sbus.R;
import android.app.Activity;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.ConnectivityManager;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;


public class PMCActivity extends Activity
{	
	private static PMCActivity s_Instance;
	private static EditText s_RDCAddress;
	private static TextView s_StatusBox;

	private OnClickListener m_MapButtonListener = new OnClickListener() {
		public void onClick(View v) {
			PhoneManagementComponent.applyMappingPolicies();
		}
	};

	private OnClickListener m_MapLocalButtonListener = new OnClickListener() {
		public void onClick(View v) {
			PhoneManagementComponent.applyMappingPoliciesLocally();
		}
	};

	private OnClickListener m_RdcButtonListener = new OnClickListener() {
		public void onClick(View v) {
			PhoneManagementComponent.informComponentsAboutRDC(true);
		}
	};

	public static String getRemoteRDCAddress() {
		return s_RDCAddress.getText().toString();
	}

	public static void addStatus(final String status) {
		s_Instance.runOnUiThread(new Runnable() {
			public void run() {
				String currentStatus = (String) s_StatusBox.getText();
				s_StatusBox.setText(status + "\n" + currentStatus);
			}
		});
	}

	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);

		s_Instance = this;

		// Display the layout.
		setContentView(R.layout.activity_pmc);

		// Add event listener to the map button.
		Button mapButton = (Button)findViewById(R.id.map_button);
		mapButton.setOnClickListener(m_MapButtonListener);

		// Apply mapping policies but only locally.
		Button mapLocalButton = (Button)findViewById(R.id.map_local_button);
		mapLocalButton.setOnClickListener(m_MapLocalButtonListener);

		// Add event listener to the rdc button.
		Button rdcButton = (Button)findViewById(R.id.rdc_button);
		rdcButton.setOnClickListener(m_RdcButtonListener);

		s_RDCAddress = (EditText)findViewById(R.id.rdc_address);
		s_StatusBox = (TextView)findViewById(R.id.status);

		new Thread() {
			public void run() {

				// Copy the needed SBUS files if they do not already exist.
				new SBUSBootloader(getApplicationContext());

				// Copy the .cpt file to the application directory.
				new FileBootloader(getApplicationContext()).store(PhoneManagementComponent.CPT_FILE);

				// Create and start the PhoneRDC.
				startService(new Intent(PMCActivity.this, PMCService.class));

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
		stopService(new Intent(PMCActivity.this, PMCService.class));
	}
}