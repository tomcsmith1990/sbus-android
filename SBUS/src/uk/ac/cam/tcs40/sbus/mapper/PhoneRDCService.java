package uk.ac.cam.tcs40.sbus.mapper;

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;

public class PhoneRDCService extends Service {

	private PhoneRDC m_PhoneRDC;
	
	@Override
	public void onCreate() {
		this.m_PhoneRDC = new PhoneRDC(getApplicationContext());
		this.m_PhoneRDC.startRDC();
	}

	@Override
	public void onDestroy() {
		this.m_PhoneRDC.stopRDC();
	}

	@Override
	public IBinder onBind(Intent arg0) {
		// TODO Auto-generated method stub
		return null;
	}
}