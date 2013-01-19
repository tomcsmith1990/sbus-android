package uk.ac.cam.tcs40.sbus.mapper;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.wifi.WifiManager;
import android.util.Log;

public class WifiReceiver extends BroadcastReceiver {

	public WifiReceiver() {
		super();
	}

	@Override
	public void onReceive(Context context, Intent intent) {
		// This is called on the main thread.
		
		if(intent.getAction().equals(WifiManager.NETWORK_STATE_CHANGED_ACTION)) {

			NetworkInfo networkInfo = intent.getParcelableExtra(WifiManager.EXTRA_NETWORK_INFO);

			if(networkInfo.isConnected()) {
				// WiFi is connected.
				Log.d(PhoneRDC.TAG, "Wifi is connected: " + String.valueOf(networkInfo));

				// Emit the map message to map once we connect to WiFi.
				PhoneRDC.remap();
				
				// Connect to the new RDC.
				PhoneRDC.registerRDC();
			}
			
		} else if(intent.getAction().equals(ConnectivityManager.CONNECTIVITY_ACTION)) {

			NetworkInfo networkInfo = intent.getParcelableExtra(ConnectivityManager.EXTRA_NETWORK_INFO);

			if(networkInfo.getType() == ConnectivityManager.TYPE_WIFI && ! networkInfo.isConnected()) {
				// WiFi is disconnected.
				Log.d(PhoneRDC.TAG, "Wifi is disconnected: " + String.valueOf(networkInfo));
			}
		}
	}
}
