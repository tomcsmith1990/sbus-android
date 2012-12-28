package uk.ac.cam.tcs40.sbus.airs.sensor;

import java.util.Date;

import uk.ac.cam.tcs40.sbus.SEndpoint;
import uk.ac.cam.tcs40.sbus.SNode;
import uk.ac.cam.tcs40.sbus.airs.sensor.dynamic.EndpointManager;

import android.os.Message;

import com.airs.platform.Acquisition;
import com.airs.platform.EventComponent;
import com.airs.platform.Sensor;
import com.airs.platform.SensorRepository;

public class AirsAcquisition extends Acquisition {

	private EndpointManager m_EptManager;

	public AirsAcquisition(EventComponent eventComponent, EndpointManager eptManager) {
		super(eventComponent);
		this.m_EptManager = eptManager;
	}

	/***********************************************************************
	 Function    : callback()
	 Input       : dialog for notification
	 Output      :
	 Return      :
	 Description : callback for subscriptions
	 ***********************************************************************/
	public void callback(DIALOG_INFO dialog)
	{
		//debug("Acquisition::callback:received method");
		//debug("...FROM  : " + new String(dialog.current_method.FROM.string));
		//debug("...TO	 : " + new String(dialog.current_method.TO.string));

		// what method type??
		switch(dialog.current_method.method_type)
		{
		case method_type.method_CONFIRM:
			// set state right in order to send further NOTIFYs
			// get return code as string
			String ret_code = new String(dialog.current_method.conf.ret_code.string);
			// positive CONFIRM?
			if (ret_code.equals("200 OK"))
			{
				dialog.dialog_state = dialog_state.SUBSCRIPTION_VALID;
				debug("...it's a positive CONFIRM - doing nothing");
			}
			else
			{
				debug("...it's a CONFIRM with code '" + ret_code + "' -> tearing down later");
			}
			// unlock dialog -> do not forget otherwise the dialog becomes unusuable!
			dialog.locked = false;								
			break;
		case method_type.method_NOTIFY:
			parseReading(dialog.current_method.event_body.string, dialog.current_method.event_body.length);
			dialog.locked = false;	
			break;
		default:
			// unlock dialog -> do not forget otherwise the dialog becomes unusuable!
			dialog.locked = false;
			debug("...there is another method - shouldn't happen");
		}
	}

	public void parseReading(byte[] reading, int length) {
		String sensorCode = new String(reading, 0, 2);

		Sensor sensor = SensorRepository.findSensor(sensorCode);

		AirsEndpoint endpoint = AirsEndpointRepository.findEndpoint(sensorCode);

		if (endpoint == null) {
			if (this.m_EptManager == null) return;
			
			String schema = "@reading { txt somestring clk timestamp int var }";
			
			if (sensor != null) {
				if (sensor.type.equals("int")) schema = "@reading { txt somestring clk timestamp int var }";
				else if (sensor.type.equals("txt")) schema = "@reading { txt somestring clk timestamp txt val }";
			} else {
				if (length == 6 && reading[2] == 0) schema = "@reading { txt somestring clk timestamp int var }";
				else schema = "@reading { txt somestring clk timestamp txt val }";
			}
			
			SEndpoint ept = this.m_EptManager.createEndpoint(sensorCode, schema);
			endpoint = new AirsEndpoint(ept, sensorCode, null);
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
			Message msg = Message.obtain(handler);
			msg.obj = s;
			handler.sendMessage(msg);
		}
	}

	public int parseInt(byte[] reading, int length) {
		int value = 0;
		value += reading[5];
		value += (reading[4] & 0xFF) << 8;
		value += (reading[3] & 0xFF) << 16;
		value += (reading[2] & 0xFF) << 24;
		return value;
	}

	public String parseString(byte[] reading, int length) {
		String value = new String(reading, 2, length - 2);
		return value;
	}
}
