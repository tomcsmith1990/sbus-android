package uk.ac.cam.tcs40.sbus.someconsumer;

import uk.ac.cam.tcs40.sbus.FileBootloader;
import uk.ac.cam.tcs40.sbus.Policy;
import uk.ac.cam.tcs40.sbus.Policy.Condition;
import uk.ac.cam.tcs40.sbus.SComponent;
import uk.ac.cam.tcs40.sbus.Policy.AIRS;
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

		// Copy the component file to the device.
		final String cptFile = "SomeConsumer.cpt";
		new FileBootloader(getApplicationContext()).store(cptFile);

		final TextView someStringTV = (TextView) findViewById(R.id.somestring);
		final TextView someValTV = (TextView) findViewById(R.id.someval);
		final TextView someVarTV = (TextView) findViewById(R.id.somevar);
		final TextView averageTV = (TextView) findViewById(R.id.average);
		
		// Consumer thread.
		new Thread() {
			public void run() {
				// Create the consumer component.
				m_Component = new SComponent("SomeConsumer", "instance");
				
				// Add the sink endpoint, allowing flexible matching.
				m_Endpoint = m_Component.addEndpoint("SomeEpt", EndpointType.EndpointSink, "BE8A47EBEB58", null, true);
				
				// 10.0.2.2 is the development machine when running in AVD.
				//scomponent.addRDC("10.0.2.2:50123");
				
				// Allow the PMC to automatically connect the component to new RDCs.
				m_Component.setRDCUpdateAutoconnect(true);
				
				// Start the component on a random port, and register with the local RDC.
				m_Component.start(getApplicationContext().getFilesDir() + "/" + cptFile, -1, true);
				
				// Add permission for components named SomeSensor to connect to this component.
				m_Component.setPermission("SomeSensor", "", true);

				// Register a map policy with the PMC for schemas containing fields with the same type as someval & somestring.
				//m_Endpoint.setAutomapPolicy("+Ssomeval+Ssomestring", "SomeEpt");
				//m_Endpoint.setAutomapPolicy(new Policy("+Ssomeval+SSomestring", "SomeEpt", AIRS.WIFI, Condition.EQUAL, 1));
				m_Endpoint.setAutomapPolicy(new Policy("+Ssomeval+SSomestring", "SomeEpt", AIRS.RANDOM, Condition.GREATER_THAN, 60000));

				SMessage message;
				SNode node;
				SEndpoint endpoint;

				// Add the endpoint to the multiplex, so that we can wait until a message is ready.
				final Multiplex multi = m_Component.getMultiplex();
				multi.add(m_Endpoint);
				
				int average = 0;
				while (m_Component != null) {

					// Wait until a message is ready.
					try {
						endpoint = multi.waitForMessage();
					} catch (Exception e) {
						// Exception thrown if waitForMessage() returns with an endpoint not on the component which owns the Multiplex.
						break;
					}

					// This will always be true for now, since it is the only endpoint in the multiplex.
					if (endpoint.getEndpointName().equals("SomeEpt")) {

						// Receive the message.
						message = m_Endpoint.receive();
						node = message.getTree();

						// Process the message.
						final String somestring = node.extractString("somestring");
						final int someval = node.extractInt("someval");
						final int somevar = node.extractInt("somevar");
						average = (average + someval) / 2;
						final int ewma = average;
						
						// Delete the message.
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
