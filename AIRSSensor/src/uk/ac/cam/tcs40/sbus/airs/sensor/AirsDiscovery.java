package uk.ac.cam.tcs40.sbus.airs.sensor;

import com.airs.platform.Discovery;
import com.airs.platform.EventComponent;
import com.airs.platform.SensorRepository;

public class AirsDiscovery extends Discovery {
	
	private SensorReadingActivity m_Activity;

	public AirsDiscovery(EventComponent current_EC, SensorReadingActivity activity) {
		super(current_EC);
		this.m_Activity = activity;
	}
	
	/***********************************************************************
	 Function    : callback()
	 Input       : dialog for notification
	 Output      :
	 Return      :
	 Description : callback for CONFIRMs of the publications
	 ***********************************************************************/
	public void callback(DIALOG_INFO dialog)
	{
		//debug("Discovery::callback:received method");
		//debug("...FROM  : " + new String(dialog.current_method.FROM.string));
		//debug("...TO	 : " + new String(dialog.current_method.TO.string));

		// what method type??
		switch(dialog.current_method.method_type)
		{
		case method_type.method_CONFIRM:
			// set state rightin order to send further NOTIFYs
			dialog.dialog_state = dialog_state.PUBLICATION_VALID;
			debug("...it's a CONFIRM - doing nothing");
			// unlock dialog -> do not forget otherwise the dialog becomes unusuable!
			dialog.locked = false;								
			break;
		case method_type.method_PUBLISH:
			if (dialog.current_method.event_body.length > 0) {

				debug("Discovery::callback: received PUBLISH with at least one sensor");
				parse(dialog.current_method.event_body.string, dialog.current_method.event_body.length, dialog.current_method.pub.Expires);
			} else {

				debug("Discovery::callback: received PUBLISH with no sensor information");
			}
			break;
		default:
			// unlock dialog -> do not forget otherwise the dialog becomes unusuable!
			dialog.locked = false;
			debug("...there is another method - shouldn't happen");
		}

	}

	public void parse(byte[] sensor_description, int length, int expires) {
		int offset = 0;
		int position = 0;

		// count lines first -> number of sensors
		while (position < length) {
			if (sensor_description[position] == '\r') {				
				// symbol::description::unit::type::scaler::min::max
				String line = new String(sensor_description, offset, position-offset);
				//System.out.println(line);
				
				// Skip over the \r
				offset = position + 1;
				
				String[] params = line.split("::");
				
				this.m_Activity.addSensor(params[0]);
				
				SensorRepository.insertSensor(params[0], 
												params[2], 
												params[1], 
												params[3], 
												Integer.parseInt(params[4]), 
												Integer.parseInt(params[5]), 
												Integer.parseInt(params[6]),
												false, 30);
			}

			position++;
		}

		//debug("Discovery::parse: found sensors: " + number_sensors);
	}

}
