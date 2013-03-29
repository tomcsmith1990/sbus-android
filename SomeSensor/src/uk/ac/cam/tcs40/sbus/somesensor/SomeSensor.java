package uk.ac.cam.tcs40.sbus.somesensor;

import uk.ac.cam.tcs40.sbus.SComponent;
import uk.ac.cam.tcs40.sbus.SComponent.EndpointType;
import uk.ac.cam.tcs40.sbus.Multiplex;
import uk.ac.cam.tcs40.sbus.SEndpoint;
import uk.ac.cam.tcs40.sbus.FileBootloader;
import uk.ac.cam.tcs40.sbus.SMessage;
import uk.ac.cam.tcs40.sbus.SNode;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.widget.TextView;
import android.os.Bundle;

public class SomeSensor extends Activity
{
	private SComponent m_Component;
	private SEndpoint m_Endpoint;

	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_some_sensor);

		// Copy the component file to the device.
		final String cptFile = "SomeSensor.cpt";
		new FileBootloader(getApplicationContext()).store("SomeSensor.cpt");

		final Activity activity = this;

		// Add a TextView to the Activity.
		final TextView tv = (TextView) findViewById(R.id.sensor_output);

		// Sensor thread.
		new Thread() {
			public void run() {
				// Create the sensor component.
				m_Component = new SComponent("SomeSensor", "instance");
				
				// Add the source endpoint.
				m_Endpoint = m_Component.addEndpoint("SomeEpt", EndpointType.EndpointSource, "BE8A47EBEB58");
				
				// 10.0.2.2 is the development machine when running in AVD.
				//scomponent.addRDC("10.0.2.2:50123");
				
				// Start the component on a random port, and register with the local RDC.
				m_Component.start(getApplicationContext().getFilesDir() + "/" + cptFile, -1, true);
				
				// Add permission for components named SomeConsumer to connect to this component.
				m_Component.setPermission("SomeConsumer", "", true);

				// Create (or get if it exists) an endpoint which receives messages about adding/removing RDCs.
				final SEndpoint rdcUpdate = m_Component.RDCUpdateNotificationsEndpoint();

				// Add the RDC update endpoint to the multiplex, so that we can wait until a message is ready.
				final Multiplex multi = m_Component.getMultiplex();
				multi.add(rdcUpdate);

				SEndpoint endpoint = null;
				int i = 0;

				while (m_Component != null) {

					// Wait two seconds until a message is ready on the RDC update endpoint.
					try {
						endpoint = multi.waitForMessage(2 * 1000000);
					} catch (Exception e) {
						// Exception thrown if waitForMessage() returns with an endpoint not on the component which owns the Multiplex.
						break;
					}

					// If there is no message, send a sensor message.
					if (endpoint == null) {
						
						SNode node;
						
						// Create the a sensor reading.
						node = m_Endpoint.createMessage("reading");
						node.packString("SomeSensor: This is message #" + i++);
						node.packInt((int) (Math.random() * 1000), "someval");
						node.packInt(34, "somevar");

						// Emit the sensor reading.
						final String s = m_Endpoint.emit(node);

						// Display the sensor reading.
						runOnUiThread(new Runnable() {
							public void run() {
								tv.setText(s);
							}
						});

					}
					// If there is a message waiting for the RDC update endpoint.
					else if (endpoint.getEndpointName().equals("rdc_update")) {

						// Receive the message.
						SMessage message = rdcUpdate.receive();
						SNode node = message.getTree();

						final String rdcAddress = node.extractString("rdc_address");
						final boolean arrived = node.extractBoolean("arrived");

						// Delete the message.
						message.delete();

						// Ask the user whether they want to add or remove the RDC.
						runOnUiThread(new Runnable() {
							public void run() {
								AlertDialog.Builder builder = new AlertDialog.Builder(activity);
								// Add the buttons
								builder.setPositiveButton("OK", new DialogInterface.OnClickListener() {
									public void onClick(DialogInterface dialog, int id) {
										// User clicked OK button
										if (arrived)
											m_Component.addRDC(rdcAddress);
										else
											m_Component.removeRDC(rdcAddress);
									}
								});
								builder.setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
									public void onClick(DialogInterface dialog, int id) {
										// User cancelled the dialog
									}
								});
								builder.setMessage((arrived ? "Add " : "Remove ") + "RDC " + rdcAddress + "?");

								// Create the AlertDialog
								AlertDialog dialog = builder.create();
								dialog.show();
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