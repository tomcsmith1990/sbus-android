package uk.ac.cam.tcs40.sbus.airs.sensor;

import java.util.Date;

import android.database.Cursor;
import android.os.Message;

public class EndpointThread implements Runnable {

	private AirsDb m_AirsDb;
	private UIHandler m_UIHandler;
	private AirsEndpoint m_Endpoint;

	public EndpointThread(AirsDb db, UIHandler uiHandler, AirsEndpoint endpoint) {
		this.m_AirsDb = db;
		this.m_UIHandler = uiHandler;
		this.m_Endpoint = endpoint;
	}

	@Override
	public void run() {
		// Open database and query for values.
		m_AirsDb = new AirsDb();
		m_AirsDb.open();
		Cursor records = m_AirsDb.query(this.m_Endpoint.getSensorCode());

		// Get column indexes.
		final int timeColumn = records.getColumnIndex("Timestamp");
		final int valueColumn = records.getColumnIndex("Value");

		// If there are records, move to the first one.
		if (!records.moveToFirst()) {
			return;
		}

		int count = 0;
		long timestamp = 0;

		while (true) {

			this.m_Endpoint.createMessage("reading");
			this.m_Endpoint.packString("AIRS: " + this.m_Endpoint.getEndpointName() + " #" + count++);
			
			timestamp = records.getLong(timeColumn);
			this.m_Endpoint.packTime(new Date(timestamp), "timestamp");

			switch (this.m_Endpoint.getValueType()) {
			case SInt:
				int i = Integer.valueOf(records.getString(valueColumn));
				this.m_Endpoint.packInt(i, this.m_Endpoint.getValueName());
				break;
			case SText:
				String s = records.getString(valueColumn);
				this.m_Endpoint.packString(s, this.m_Endpoint.getValueName());
			}

			final String s = this.m_Endpoint.emit();

			Message msg = Message.obtain(m_UIHandler);
			msg.obj = s;
			m_UIHandler.sendMessage(msg);

			// Move to next record if possible.
			// If not, go back to start.
			if (!records.moveToNext()) records.moveToFirst();

			try {
				Thread.sleep(2000);
			} catch (InterruptedException e) { }
		}

	}

}
