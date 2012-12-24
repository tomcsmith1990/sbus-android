/*
Copyright (C) 2005-2006 Nokia Corporation
Copyright (C) 2008-2011, Dirk Trossen, airs@dirk-trossen.de

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation as version 2.1 of the License.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
59 Temple Place, Suite 330, Boston, MA 02111-1307 USA 
 */
package com.airs.platform;

/**
 * @author trossen
 * @date Nov 28, 2004
 * 
 * Purpose: 
 */
public class Discovery implements Callback
{
	private EventComponent 	current_EC;
	private DIALOG_INFO		dialog=null;
	private int 			polltime=15000;
	private String			TO = new String("REMONT AS");
	private String  		event_name = new String("available");

	protected static void debug(String msg) 
	{
		System.out.println(msg);
	}

	/**
	 * Sleep function 
	 * @param millis
	 */
	protected static void sleep(long millis) 
	{
		try 
		{
			Thread.sleep(millis);
		} 
		catch (InterruptedException ignore) 
		{
		}
	}

	/***********************************************************************
	 Function    : Discovery()
	 Input       : 
	 Output      :
	 Return      :
	 Description : constructor of class
	 ***********************************************************************/
	public Discovery(EventComponent current_EC)
	{
		this.current_EC = current_EC;

		// register acquisition event server
		if (this.current_EC.registerEventServer(this, event_name)==false)
			debug("Discovery::Discovery(): failure in registering 'available' event");
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
		debug("Discovery::callback:received method");
		debug("...FROM  : " + new String(dialog.current_method.FROM.string));
		debug("...TO	 : " + new String(dialog.current_method.TO.string));

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
		int number_sensors = 0;
		int offset = 0;
		int position = 0;

		// count lines first -> number of sensors
		while (position < length) {
			if (sensor_description[position] == '\r') {
				number_sensors++;
				
				// symbol::description::unit::type::scaler::min::max
				String line = new String(sensor_description, offset, position-offset);
				System.out.println(line);
				
				// Skip over the \r
				offset = position + 1;
				
				String[] params = line.split("::");
				
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

		debug("Discovery::parse: found sensors: " + number_sensors);


	}
}
