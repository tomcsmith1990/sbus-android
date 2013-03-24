package uk.ac.cam.tcs40.sbus.someconsumer;

import uk.ac.cam.tcs40.sbus.FileBootloader;
import uk.ac.cam.tcs40.sbus.SComponent;
import uk.ac.cam.tcs40.sbus.SComponent.EndpointType;
import uk.ac.cam.tcs40.sbus.Multiplex;
import uk.ac.cam.tcs40.sbus.SEndpoint;
import uk.ac.cam.tcs40.sbus.SMessage;
import uk.ac.cam.tcs40.sbus.SNode;
import android.os.Bundle;
import android.app.Activity;
import android.widget.TextView;

public class SomeConsumer extends Activity {

	private SComponent m_Component;
	private SEndpoint m_Endpoint;
	
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_some_consumer);

		// Create a FileBootloader to store our component file.
		new FileBootloader(getApplicationContext()).store("SomeConsumer.cpt");

		final TextView someStringTV = (TextView) findViewById(R.id.somestring);
		final TextView someValTV = (TextView) findViewById(R.id.someval);
		final TextView someVarTV = (TextView) findViewById(R.id.somevar);
		final TextView averageTV = (TextView) findViewById(R.id.average);
		
		// Create a thread to run the sensor.
		new Thread() {
			public void run() {
				m_Component = new SComponent("SomeConsumer", "instance");
				m_Endpoint = m_Component.addEndpoint("SomeEpt", EndpointType.EndpointSink, "BE8A47EBEB58", null, true);
				// 10.0.2.2 is the development machine when running in AVD.
				//scomponent.addRDC("10.0.2.2:50123");
				final String cptFile = "SomeConsumer.cpt";
				m_Component.setRDCUpdateAutoconnect(true);
				m_Component.start(getApplicationContext().getFilesDir() + "/" + cptFile, -1, true);
				m_Component.setPermission("SomeSensor", "", true);

				m_Endpoint.setAutomapPolicy("+Ssomeval+Ssomestring", "SomeEpt");

				SMessage message;
				SNode node;
				SEndpoint endpoint;

				final Multiplex multi = m_Component.getMultiplex();
				multi.add(m_Endpoint);
				
				int average = 0;
				while (m_Component != null) {

					try {
						endpoint = multi.waitForMessage();
					} catch (Exception e) {
						// Exception thrown if waitForMessage() returns with an endpoint not on the component which owns the Multiplex.
						break;
					}

					if (endpoint.getEndpointName().equals("SomeEpt")) {

						message = m_Endpoint.receive();
						node = message.getTree();

						final String somestring = node.extractString("somestring");
						final int someval = node.extractInt("someval");
						final int somevar = node.extractInt("somevar");
						average = (average + someval) / 2;
						final int ewma = average;
						message.delete();

						runOnUiThread(new Runnable() {
							public void run() {
								someStringTV.setText("somestring: " + somestring);
								someValTV.setText("someval: " + someval);
								someVarTV.setText("somevar: " + somevar);
								averageTV.setText("moving average: " + ewma);
							}
						});	
					}
				}
			}
		}.start();
	}
	
	@Override
	public void onDestroy() {
		super.onDestroy();
		m_Endpoint.unmap();
		m_Component.delete();
		m_Component = null;
	}
}
