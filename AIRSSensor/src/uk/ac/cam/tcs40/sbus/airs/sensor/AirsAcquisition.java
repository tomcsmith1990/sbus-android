package uk.ac.cam.tcs40.sbus.airs.sensor;

import java.util.Date;

import uk.ac.cam.tcs40.sbus.SEndpoint;
import uk.ac.cam.tcs40.sbus.SNode;
import uk.ac.cam.tcs40.sbus.airs.sensor.dynamic.EndpointManager;
import uk.ac.cam.tcs40.sbus.airs.sensor.dynamic.UIManager;

import android.os.Message;

import com.airs.platform.Acquisition;
import com.airs.platform.Callback;
import com.airs.platform.EventComponent;
import com.airs.platform.Sensor;
import com.airs.platform.SensorRepository;

public class AirsAcquisition extends Acquisition implements Callback {

	private EndpointManager m_EptManager;

	public AirsAcquisition(EventComponent eventComponent, EndpointManager eptManager) {
		super(eventComponent);
		this.m_EptManager = eptManager;
	}

	public void callback(DIALOG_INFO dialog)
	{
		if (dialog.current_method.method_type == method_type.method_NOTIFY) {
			parseReading(dialog.current_method.event_body.string, dialog.current_method.event_body.length);
		}
		// unlock dialog -> do not forget otherwise the dialog becomes unusuable!
		dialog.locked = false;
	}

	@Override
	protected void parseReading(byte[] reading, int length) {
		String sensorCode = new String(reading, 0, 2);

		Sensor sensor = SensorRepository.findSensor(sensorCode);

		AirsEndpoint endpoint = AirsEndpointRepository.findEndpoint(sensorCode);

		if (endpoint == null) {
			if (this.m_EptManager == null) return;

			final String schema = "@reading { txt sensor clk timestamp [ int var ] [ txt val ] }";

			SEndpoint ept = this.m_EptManager.createEndpoint(sensorCode, schema);
			endpoint = new AirsEndpoint(ept, sensorCode, UIManager.getInstance().getUIHandler());
			AirsEndpointRepository.addEndpoint(endpoint);
			
			ept.map(":50123", "AIRS");
		}

		SNode node = endpoint.getEndpoint().createMessage("reading");
		node.packString(sensorCode, "sensor");

		node.packTime(new Date(), "timestamp");

		if (sensor != null) {

			if (sensor.type.equals("int")) {

				node.packInt(parseInt(reading, length), "var");
				node.packString(null, "val");

			} else if (sensor.type.equals("txt")) {

				node.packInt(0, "var");
				node.packString(parseString(reading, length), "val");
			}
		} else {
			if (length == 6 && reading[2] == 0) {
				// guess that it is an int.
				node.packInt(parseInt(reading, length), "var");
				node.packString(null, "val");

			} else {
				node.packInt(0, "var");
				node.packString(parseString(reading, length), "val");
			}
		}

		final String s = endpoint.getEndpoint().emit(node);

		UIHandler handler = endpoint.getUIHandler();
		if (handler != null) {
			Message mesage = Message.obtain(handler);
			mesage.obj = s;
			handler.sendMessage(mesage);
		}
	}

	@Override
	protected int parseInt(byte[] reading, int length) {
		int value = 0;
		value += reading[5];
		value += (reading[4] & 0xFF) << 8;
		value += (reading[3] & 0xFF) << 16;
		value += (reading[2] & 0xFF) << 24;
		return value;
	}

	@Override
	protected String parseString(byte[] reading, int length) {
		String value = new String(reading, 2, length - 2);
		return value;
	}
}
