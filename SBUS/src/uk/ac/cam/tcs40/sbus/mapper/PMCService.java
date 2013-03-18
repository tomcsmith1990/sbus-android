package uk.ac.cam.tcs40.sbus.mapper;

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;

public class PMCService extends Service {

	private PhoneManagementComponent m_PMC;
	
	@Override
	public void onCreate() {
		this.m_PMC = new PhoneManagementComponent(getApplicationContext());
		this.m_PMC.start();
	}

	@Override
	public void onDestroy() {
		this.m_PMC.stop();
	}

	@Override
	public IBinder onBind(Intent arg0) {
		// TODO Auto-generated method stub
		return null;
	}
}