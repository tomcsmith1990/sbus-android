package uk.ac.cam.tcs40.sbus;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.wifi.WifiManager;
import android.util.Log;

public class WifiReceiver extends BroadcastReceiver {

	private SComponent m_mapComponent;

	public WifiReceiver(SComponent mapComponent) {
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

				this.m_mapComponent.endpointMap(":44444");

				this.m_mapComponent.createMessage("map");
				this.m_mapComponent.packString("SomeEpt", "endpoint");
				this.m_mapComponent.packString("192.168.0.6:44444", "peer_address");
				this.m_mapComponent.packString("SomeEpt", "peer_endpoint");
				this.m_mapComponent.packString("", "certificate");

				this.m_mapComponent.emit();
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
