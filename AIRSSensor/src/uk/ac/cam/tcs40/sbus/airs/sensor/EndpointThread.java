package uk.ac.cam.tcs40.sbus.airs.sensor;

import java.util.Date;

import android.database.Cursor;
import android.os.Message;

public class EndpointThread implements Runnable {

	private AirsDb m_AirsDb;
	private String m_SensorCode;

	public EndpointThread(AirsDb db, String sensorCode) {
		this.m_AirsDb = db;
	}

	@Override
	public void run() {
		// Open database and query for values.
		m_AirsDb = new AirsDb();
		m_AirsDb.open();
		Cursor records = m_AirsDb.query(this.m_SensorCode);

		// Get column indexes.
		final int timeColumn = records.getColumnIndex("Timestamp");
		final int valueColumn = records.getColumnIndex("Value");

		// If there are records, move to the first one.
		if (!records.moveToFirst()) {
			return;
		}

		int count = 0;
		long timestamp = 0;

		AirsEndpoint endpoint = AirsEndpointRepository.findEndpoint(this.m_SensorCode);
		if (endpoint != null) {
			
			while (true) {

				endpoint.createMessage("reading");
				endpoint.packString("AIRS: " + endpoint.getEndpointName() + " #" + count++);

				timestamp = records.getLong(timeColumn);
				endpoint.packTime(new Date(timestamp), "timestamp");

				switch (endpoint.getValueType()) {
				case SInt:
					int i = Integer.valueOf(records.getString(valueColumn));
					endpoint.packInt(i, endpoint.getValueName());
					break;
				case SText:
					String s = records.getString(valueColumn);
					endpoint.packString(s, endpoint.getValueName());
				}

				final String s = endpoint.emit();

				UIHandler handler = endpoint.getUIHandler();
				Message msg = Message.obtain(handler);
				msg.obj = s;
				handler.sendMessage(msg);

				// Move to next record if possible.
				// If not, go back to start.
				if (!records.moveToNext()) records.moveToFirst();

				try {
					Thread.sleep(2000);
				} catch (InterruptedException e) { }
			}
		}
	}

}
