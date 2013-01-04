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

	private void parseReading(byte[] reading, int length) {
		String sensorCode = new String(reading, 0, 2);

		Sensor sensor = SensorRepository.findSensor(sensorCode);

		AirsEndpoint endpoint = AirsEndpointRepository.findEndpoint(sensorCode);

		if (endpoint == null) {
			if (this.m_EptManager == null) return;

			final String intSchema = "@reading { txt somestring clk timestamp int var }";
			final String txtSchema = "@reading { txt somestring clk timestamp txt val }";
			String schema = intSchema;

			if (sensor != null) {
				if (sensor.type.equals("int")) schema = intSchema;
				else if (sensor.type.equals("txt")) schema = txtSchema;
			} else {
				if (length == 6 && reading[2] == 0) schema = intSchema;
				else schema = txtSchema;
			}

			SEndpoint ept = this.m_EptManager.createEndpoint(sensorCode, schema);
			endpoint = new AirsEndpoint(ept, sensorCode, UIManager.getInstance().getUIHandler());
			AirsEndpointRepository.addEndpoint(endpoint);
		}

		SNode node = endpoint.getEndpoint().createMessage("reading");
		node.packString("AIRS: " + endpoint.getEndpoint().getEndpointName());

		node.packTime(new Date(), "timestamp");

		//System.out.println("...SENSOR : " + sensorCode);

		if (sensor != null) {
			//System.out.println("...DESCRIPTION: " + sensor.Description);

			if (sensor.type.equals("int")) {

				node.packInt(parseInt(reading, length), "var");

			} else if (sensor.type.equals("txt")) {

				node.packString(parseString(reading, length), "val");
			}
		} else {
			if (length == 6 && reading[2] == 0) {
				// guess that it is an int.
				node.packInt(parseInt(reading, length), "var");

			} else {
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
}
