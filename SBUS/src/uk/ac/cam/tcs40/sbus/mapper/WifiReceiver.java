package uk.ac.cam.tcs40.sbus.mapper;

import android.annotation.SuppressLint;
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
				PhoneManagementComponent.setPhoneIP(formatIP(ip));
				
				// Status just to say what WiFi network we're on.
				PMCActivity.addStatus("Wi-Fi: " + wifiInfo.getSSID());
				
				// Connect components to the new RDC and apply mapping policies.
				PhoneManagementComponent.informComponentsAboutRDC(true);
			}

		} else if(intent.getAction().equals(ConnectivityManager.CONNECTIVITY_ACTION)) {

			NetworkInfo networkInfo = intent.getParcelableExtra(ConnectivityManager.EXTRA_NETWORK_INFO);

			if(networkInfo.getType() == ConnectivityManager.TYPE_WIFI && !networkInfo.isConnected()) {
				// WiFi is disconnected.
				PhoneManagementComponent.setPhoneIP("127.0.0.1");
				PhoneManagementComponent.informComponentsAboutRDC(false);
			}
		}
	}

	@SuppressLint("DefaultLocale")
	private String formatIP(int ip) {
		return String.format("%d.%d.%d.%d", (ip & 0xff), (ip >> 8 & 0xff), (ip >> 16 & 0xff), (ip >> 24 & 0xff));
	}
}
