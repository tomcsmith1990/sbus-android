package uk.ac.cam.tcs40.sbus;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.wifi.WifiManager;
import android.util.Log;

public class WifiReceiver extends BroadcastReceiver {

	private MapEndpoint m_MapEndpoint;

	public WifiReceiver(MapEndpoint mapEndpoint) {
		super();
		this.m_MapEndpoint = mapEndpoint;
	}

	@Override
	public void onReceive(Context context, Intent intent) {

		if(intent.getAction().equals(WifiManager.NETWORK_STATE_CHANGED_ACTION)) {

			NetworkInfo networkInfo = intent.getParcelableExtra(WifiManager.EXTRA_NETWORK_INFO);

			if(networkInfo.isConnected()) {
				// WiFi is connected.
				Log.d("SBUS", "Wifi is connected: " + String.valueOf(networkInfo));

				// Emit the map message to map once we connect to WiFi.
				this.m_MapEndpoint.map(":44445", "Random", "192.168.0.3:44444", "Random");
			}
			
		} else if(intent.getAction().equals(ConnectivityManager.CONNECTIVITY_ACTION)) {

			NetworkInfo networkInfo = intent.getParcelableExtra(ConnectivityManager.EXTRA_NETWORK_INFO);

			if(networkInfo.getType() == ConnectivityManager.TYPE_WIFI && ! networkInfo.isConnected()) {
				// WiFi is disconnected.
				Log.d("SBUS", "Wifi is disconnected: " + String.valueOf(networkInfo));
			}
		}
	}
}
