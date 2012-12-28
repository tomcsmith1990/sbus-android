package uk.ac.cam.tcs40.sbus.airs.sensor.dynamic;

import uk.ac.cam.tcs40.sbus.airs.sensor.R;
import uk.ac.cam.tcs40.sbus.airs.sensor.UIHandler;
import android.app.Activity;
import android.widget.LinearLayout;
import android.widget.TextView;

public class UIManager {

	private static UIManager s_Instance;

	private Activity m_Activity;
	private final MessageQueue<UIHandler> m_Queue = new SafeMessageQueue<UIHandler>();

	private UIManager(Activity activity) {
		m_Activity = activity;
	}

	public static UIManager getInstance() {
		return s_Instance;
	}

	public static UIManager getInstance(Activity activity) {
		if (s_Instance == null) s_Instance = new UIManager(activity);
		return s_Instance;
	}

	public void createTextView() {
		if (m_Activity == null) return;

		m_Activity.runOnUiThread(new Runnable() {

			@Override
			public void run() {
				TextView tv = new TextView(m_Activity.getApplicationContext());
				LinearLayout layout = (LinearLayout) m_Activity.findViewById(R.id.output);
				layout.addView(tv);

				addTextView(tv);
			}
		});
	}

	private void addTextView(TextView tv) {
		m_Queue.put(new UIHandler(tv));
	}

	public UIHandler getUIHandler() {
		createTextView();
		return m_Queue.take();
	}
}
