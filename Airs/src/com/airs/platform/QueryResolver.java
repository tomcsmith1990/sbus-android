/*
Copyright (C) 2004-2006 Nokia Corporation
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
 * @date Nov 18, 2004
 * 
 * Purpose: This class is instantiated for each incoming query
 */
public class QueryResolver implements Runnable
{
	public boolean terminate = false;
	private short dialog_id;
	private byte[] Query;
	private String query;
	private boolean extracted = false;
	private EventComponent current_EC;
	private Acquisition current_Acquisition;
	// sleep time of query resolver to see if query is fulfilled
	private int sleeptime		= -1;
	
	protected void sleep(long millis) 
	{
		try 
		{
			Thread.sleep(millis);
		} 
		catch (InterruptedException ignore) 
		{
		}
	}

	protected void debug(String msg) 
	{
		System.out.println(msg);
	}

	/***********************************************************************
	 Function    : QueryResolver()
	 Input       : EventComponent, Acquisition Component, dialog_id and query 
	 			   for the subscription
	 Output      :
	 Return      :
	 Description : stores parameters of this query
	***********************************************************************/
	QueryResolver(EventComponent current_EC, Acquisition current_Acquisition, short dialog_id, byte Query[])
	{
		// copy parameters 
		this.current_EC 		 = current_EC;
		this.current_Acquisition = current_Acquisition;
		this.dialog_id 			 = dialog_id;
		this.Query 				 = Query;	
	}

	/***********************************************************************
	 Function    : run()
	 Input       : 
	 Output      :
	 Return      :
	 Description : thread for resolving a query - to be started by the 
	 			   Acquisition component (usually in the callback for a dialog)!!
	 			   if sending NOTIFY fails, thread returns, i.e., ends
	 			   NOTIFY could fail due to termination of dialog (e.g., BYE) 
	***********************************************************************/
	public void run() 
	{
		Sensor sensor = null; 
		byte values[];

		// run until to be terminated by the Acquisition component
		while(terminate==false)
		{
			// parse query
			sensor = parse(Query);

			// is the sensor discovered?
			if (sensor != null)
			{
				// get sleeptime from sensor data if not done in query
				if (sleeptime == -1)
					sleeptime = sensor.polltime;
				// get current data
				values = sensor.get_value(query);
				// anything there?
				if (values != null)
				{
					debug("QueryResolver:NOTIFY for dialog_id " + dialog_id);
					if (current_EC.Notify(dialog_id, values, current_Acquisition)==false)
						return;
				}
			}
			else
				sleeptime = 1000;
			// sleep predefined time until checking again
			sleep(sleeptime);
			
			// dereference for garbage collector
			values = null;
			sensor = null;
		}
		
		debug("QueryResolver::run:terminated");
	}
	
	/***********************************************************************
	 Function    : set_polltime()
	 Input       : polltime
	 Output      :
	 Return      : none
	 Description : change polltime of individual query.  
	***********************************************************************/
	private void set_polltime(int poll)
	{
		sleeptime = poll;
	}
	
	/***********************************************************************
	 Function    : parse()
	 Input       : query string
	 Output      :
	 Return      : true = valid query; false otherwise
	 Description : parses incoming query for resource availability and syntactial
	               correctness (later).  
	***********************************************************************/
	Sensor parse(byte[] Query)
	{
		int polltime;
		Sensor sensor = null;
		int query_index;
		// form query string to be parsed
		String parse = new String(Query);
		// extract first two characters for sensor symbol
		String symbol = parse.substring(0, 2);
		
		// try to find ':' for additional info, if not yet done, and extract rest of query (only once)
		if (extracted == false)
		{
			query_index = parse.indexOf(":");
			if (query_index>0)				
				try
				{
					query = parse.substring(query_index + 1);
				}
				catch(Exception e)
				{
					query = null;
				}
			extracted = true;
		}
		
		// this is rather simple right now and needs extension towards the full blown TinyML and XQuery model
		// for now, the query is assumed to have the sensor symbol, which is looked up in the repository. 
		// if it exists at the time of subscription, the query is valid, otherwise not
		if ((sensor = SensorRepository.findSensor(symbol)) != null)
		{
			// right now, only a syntax "symbol:polltime" is supported!
			if (query != null)
			{
				// parse polltime from query
				try
				{
					polltime = Integer.parseInt(query);
					// change polltime both in query resolver and sensor to work properly!!
					set_polltime(polltime);
					sensor.set_polltime(polltime);			
				}
				catch(Exception e)
				{
					
				}
			}
		}
		
		// garbage collection
		parse = null;
		symbol = null;

		return sensor;
	}

	/***********************************************************************
	 Function    : s_parse()
	 Input       : query string for sensor symbol and look up sensor
	 Output      :
	 Return      : sensor 
	 Description : parses incoming query for resource availability.  
	***********************************************************************/
	static Sensor s_parse(byte[] Query)
	{
		// form query to be parsed
		String parse = new String(Query);
		// extract first two symbols
		String symbol = parse.substring(0, 2);
		
		return SensorRepository.findSensor(symbol);
	}

}
