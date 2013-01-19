package uk.ac.cam.tcs40.sbus.mapper;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;

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

				WifiManager wifiManager = (WifiManager) context.getSystemService(Context.WIFI_SERVICE);
				WifiInfo wifiInfo = wifiManager.getConnectionInfo();
				int ip = wifiInfo.getIpAddress();

				// Set the current IP in phone RDC so we only register components on the phone.
				PhoneRDC.setIP(formatIP(ip));
				
				// Emit the map message to map once we connect to WiFi.
				PhoneRDC.remap();

				// Connect to the new RDC.
				PhoneRDC.registerRDC();
			}

		} else if(intent.getAction().equals(ConnectivityManager.CONNECTIVITY_ACTION)) {

			NetworkInfo networkInfo = intent.getParcelableExtra(ConnectivityManager.EXTRA_NETWORK_INFO);

			if(networkInfo.getType() == ConnectivityManager.TYPE_WIFI && ! networkInfo.isConnected()) {
				// WiFi is disconnected.
				PhoneRDC.setIP("127.0.0.1");
			}
		}
	}

	private String formatIP(int ip) {
		return String.format(
				"%d.%d.%d.%d",
				(ip & 0xff),
				(ip >> 8 & 0xff),
				(ip >> 16 & 0xff),
				(ip >> 24 & 0xff));
	}
}
