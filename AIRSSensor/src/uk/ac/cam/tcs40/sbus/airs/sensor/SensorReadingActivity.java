package uk.ac.cam.tcs40.sbus.airs.sensor;

import java.util.LinkedList;
import java.util.List;

import com.airs.platform.Sensor;

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
	private ListView m_SensorList;
	private ArrayAdapter<Sensor> m_Adapter;

	private List<Sensor> m_Sensors = new LinkedList<Sensor>();

	private OnItemClickListener m_ListClick = new OnItemClickListener() {
		@Override
		public void onItemClick(AdapterView<?> parent, android.view.View view, int position, long id) {
			if (m_SensorList != null && m_Gateway != null && m_Adapter != null) {

				if (m_SensorList.getCheckedItemPositions().get(position)) {
					// If we have just checked the sensor, subscribe.
					m_Gateway.subscribe(m_Adapter.getItem(position).Symbol);
				}
			}
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
		this.m_SensorList = (ListView) findViewById(R.id.sensors);
		this.m_Adapter = new ArrayAdapter<Sensor>(this, android.R.layout.simple_list_item_multiple_choice, m_Sensors);
		this.m_SensorList.setAdapter(this.m_Adapter);
		this.m_SensorList.setOnItemClickListener(m_ListClick);

		// Create the component.
		this.m_Gateway = new AirsSbusGateway(this);
		new Thread() {
			public void run() {
				// Start waiting for AIRS to connect.
				m_Gateway.startReadings();
			}
		}.start();
	}

	public void onDestroy() {
		super.onDestroy();
		if (m_Gateway != null)
			m_Gateway.stop();
	}

	public void addSensorToList(final Sensor sensor) {
		runOnUiThread(new Runnable() {
			@Override
			public void run() {
				m_Adapter.add(sensor);
				m_Adapter.notifyDataSetChanged();
			}
		});
	}

	public void setStatusText(final String message) {
		if (m_StatusTextView == null) return;

		runOnUiThread(new Runnable() {
			public void run() {
				m_StatusTextView.setText(message);
			}
		});
	}
}