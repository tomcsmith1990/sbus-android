package uk.ac.cam.tcs40.sbus.airs.sensor;

import android.os.Handler;
import android.os.Message;
import android.widget.TextView;

public class UIHandler extends Handler {
	
	private TextView m_TextView;
	
	public UIHandler(TextView tv) {
		super();
		this.m_TextView = tv;
	}
	
	@Override
    public void handleMessage(Message msg) {
        // a message is received; update UI text view
		this.m_TextView.setText(msg.obj.toString());
        super.handleMessage(msg);
    }
}
