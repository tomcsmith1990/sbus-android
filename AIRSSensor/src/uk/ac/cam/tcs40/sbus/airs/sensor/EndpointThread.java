package uk.ac.cam.tcs40.sbus.airs.sensor;

import java.util.Date;

import uk.ac.cam.tcs40.sbus.SEndpoint;
import android.database.Cursor;
import android.os.Message;

public class EndpointThread implements Runnable {

	private AirsDb m_AirsDb;
	private UIHandler m_UIHandler;
	private SEndpoint m_Endpoint;

	public EndpointThread(AirsDb db, UIHandler uiHandler, SEndpoint endpoint) {
		this.m_AirsDb = db;
		this.m_UIHandler = uiHandler;
		this.m_Endpoint = endpoint;
	}

	@Override
	public void run() {
		int i = 0;

		// Open database and query for values.
		m_AirsDb = new AirsDb();
		m_AirsDb.open();
		Cursor values = m_AirsDb.query("BV");

		// Records from database.
		int batteryVoltage = -1;
		long timestamp = 0;

		// Get column indexes.
		final int timeColumn = values.getColumnIndex("Timestamp");
		final int valueColumn = values.getColumnIndex("Value");

		// If there are records, read the first one.
		if (values.moveToFirst()) {
			batteryVoltage = Integer.valueOf(values.getString(valueColumn));
			timestamp = values.getLong(timeColumn);
		}

		while (true) {

			this.m_Endpoint.createMessage("reading");
			this.m_Endpoint.packString("AIRS: " + this.m_Endpoint.getEndpointName() + " #" + i++);
			this.m_Endpoint.packTime(new Date(timestamp), "timestamp");
			this.m_Endpoint.packInt(batteryVoltage, "batteryVoltage");

			final String s = this.m_Endpoint.emit();

			Message msg = Message.obtain(m_UIHandler);
			msg.obj = s;
			m_UIHandler.sendMessage(msg);

			// Move to and read next record if possible.
			// If not, go back to start.
			if (!values.moveToNext()) values.moveToFirst();

			batteryVoltage = Integer.valueOf(values.getString(valueColumn));
			timestamp = values.getLong(timeColumn);

			try {
				Thread.sleep(2000);
			} catch (InterruptedException e) { }
		}

	}

}
