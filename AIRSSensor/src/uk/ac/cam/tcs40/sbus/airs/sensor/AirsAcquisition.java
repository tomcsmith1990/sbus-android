package uk.ac.cam.tcs40.sbus.airs.sensor;

import java.util.Date;

import android.os.Message;

import com.airs.platform.Acquisition;
import com.airs.platform.EventComponent;
import com.airs.platform.Sensor;
import com.airs.platform.SensorRepository;

public class AirsAcquisition extends Acquisition {

	private UIHandler m_UIHandler;
	private AirsEndpoint m_Endpoint;

	public AirsAcquisition(EventComponent current_EC, UIHandler uiHandler, AirsEndpoint endpoint) {
		super(current_EC);
		this.m_UIHandler = uiHandler;
		this.m_Endpoint = endpoint;
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

		//System.out.println("...SENSOR : " + sensorCode);

		this.m_Endpoint.createMessage("reading");
		this.m_Endpoint.packString("AIRS: " + this.m_Endpoint.getEndpointName());

		this.m_Endpoint.packTime(new Date(), "timestamp");


		if (sensor != null) {
			//System.out.println("...DESCRIPTION: " + sensor.Description);

			if (sensor.type.equals("int")) {

				this.m_Endpoint.packInt(parseInt(reading, length), this.m_Endpoint.getValueName());

			} else if (sensor.type.equals("txt")) {

				this.m_Endpoint.packString(parseString(reading, length), this.m_Endpoint.getValueName());
			}
		} else {
			if (length == 6 && reading[2] == 0) {
				// guess that it is an int.
				this.m_Endpoint.packInt(parseInt(reading, length), this.m_Endpoint.getValueName());
			} else {
				this.m_Endpoint.packString(parseString(reading, length), this.m_Endpoint.getValueName());
			}
		}

		final String s = this.m_Endpoint.emit();

		Message msg = Message.obtain(m_UIHandler);
		msg.obj = s;
		m_UIHandler.sendMessage(msg);
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
