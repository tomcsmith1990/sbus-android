package uk.ac.cam.tcs40.sbus.airs.sensor;

import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;

public class AirsEndpointRepository {
	static private List<AirsEndpoint> list = null;

	static synchronized public void addEndpoint(AirsEndpoint endpoint)
	{
		if (list == null)
			list = new LinkedList<AirsEndpoint>();

		list.add(endpoint);
	}

	static synchronized public void removeEndpoint(String sensorCode)
	{
		Iterator<AirsEndpoint> it = list.iterator();

		AirsEndpoint current;

		while(it.hasNext())
		{
			current = it.next();
			if (current.getSensorCode().equals(sensorCode)) {
				list.remove(current);
				return;
			}
		}
	}

	static synchronized public List<String> getSensorCodes() {
		List<String> sensorCodes = new LinkedList<String>();

		Iterator<AirsEndpoint> it = list.iterator();

		while(it.hasNext())
		{
			sensorCodes.add(it.next().getSensorCode());
		}
		return sensorCodes;
	}

	static synchronized public AirsEndpoint findEndpoint(String sensorCode)
	{
		Iterator<AirsEndpoint> it = list.iterator();

		AirsEndpoint current;

		while(it.hasNext())
		{
			current = it.next();
			if (current.getSensorCode().equals(sensorCode))
				return current;
		}
		return null;
	}
}
