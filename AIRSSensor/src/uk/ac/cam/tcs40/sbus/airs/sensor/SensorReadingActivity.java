package uk.ac.cam.tcs40.sbus.airs.sensor;

import uk.ac.cam.tcs40.sbus.airs.sensor.dynamic.UIManager;
import android.os.Bundle;
import android.app.Activity;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.TextView;

public class SensorReadingActivity extends Activity {

	private AirsSbusGateway m_Gateway;
	private TextView m_StatusTextView;

	private String[] m_Sensors = new String[] { "Rd", "BV", "VC", "Rm" };
	
	private OnItemClickListener m_ListClick = new OnItemClickListener() {
		@Override
		public void onItemClick(AdapterView<?> parent, android.view.View view, int position, long id) {
			m_Gateway.subscribe(m_Sensors[position]);
		}
	};

	/** Called when the activity is first created. */
	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_airs);

		// Set this as the activity so we can create TextViews.
		UIManager.getInstance(this);

		// The status TextView.
		this.m_StatusTextView = (TextView) findViewById(R.id.status);

		// The sensor ListView.
		ListView list = (ListView) findViewById(R.id.sensors);
		ArrayAdapter<String> adapter = new ArrayAdapter<String>(this,
				android.R.layout.simple_list_item_1, android.R.id.text1, m_Sensors);
		list.setAdapter(adapter);
		list.setOnItemClickListener(m_ListClick);

		this.m_Gateway = new AirsSbusGateway(this);
		this.m_Gateway.startReadings();
	}

	public void setStatusText(final String message) {
		runOnUiThread(new Runnable() {
			public void run() {
				m_StatusTextView.setText(message);
			}
		});
	}
}