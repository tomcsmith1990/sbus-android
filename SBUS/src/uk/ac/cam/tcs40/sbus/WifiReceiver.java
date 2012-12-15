package uk.ac.cam.tcs40.sbus;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.wifi.WifiManager;
import android.util.Log;

public class WifiReceiver extends BroadcastReceiver {

	private MapComponent m_mapComponent;

	public WifiReceiver(MapComponent mapComponent) {
		super();
		this.m_mapComponent = mapComponent;
	}

	@Override
	public void onReceive(Context context, Intent intent) {

		if(intent.getAction().equals(WifiManager.NETWORK_STATE_CHANGED_ACTION)) {
			
			NetworkInfo networkInfo = intent.getParcelableExtra(WifiManager.EXTRA_NETWORK_INFO);
			
			if(networkInfo.isConnected()) {
				// Wifi is connected
				Log.d("SBUS", "Wifi is connected: " + String.valueOf(networkInfo));

				this.m_mapComponent.map(":44444", "SomeEpt", "192.168.0.6:44444", "SomeEpt");
							}
		} else if(intent.getAction().equals(ConnectivityManager.CONNECTIVITY_ACTION)) {
			
			NetworkInfo networkInfo = intent.getParcelableExtra(ConnectivityManager.EXTRA_NETWORK_INFO);
			
			if(networkInfo.getType() == ConnectivityManager.TYPE_WIFI && ! networkInfo.isConnected()) {
				// Wifi is disconnected
				Log.d("SBUS", "Wifi is disconnected: " + String.valueOf(networkInfo));
			}
		}
	}
}
