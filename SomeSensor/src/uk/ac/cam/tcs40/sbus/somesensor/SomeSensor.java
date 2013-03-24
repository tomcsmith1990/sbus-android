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
import android.util.Log;
import android.widget.TextView;
import android.os.Bundle;

public class SomeSensor extends Activity
{
	private SComponent m_Component;
	private SEndpoint m_Endpoint;

	/** Called when the activity is first created. */
	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);

		// Create a FileBootloader to store our component file.
		new FileBootloader(getApplicationContext()).store("SomeSensor.cpt");

		final Activity activity = this;

		// Add a TextView to the Activity.
		final TextView tv = new TextView(this);
		tv.setText("Some Sensor");
		setContentView(tv);

		// Create a thread to run the sensor.
		new Thread() {
			public void run() {
				m_Component = new SComponent("SomeSensor", "instance");
				m_Endpoint = m_Component.addEndpoint("SomeEpt", EndpointType.EndpointSource, "BE8A47EBEB58");
				// 10.0.2.2 is the development machine when running in AVD.
				//scomponent.addRDC("10.0.2.2:50123");
				String cptFile = "SomeSensor.cpt";
				m_Component.start(getApplicationContext().getFilesDir() + "/" + cptFile, -1, true);
				m_Component.setPermission("SomeConsumer", "", true);

				final SEndpoint rdcUpdate = m_Component.RDCUpdateNotificationsEndpoint();

				final Multiplex multi = m_Component.getMultiplex();
				multi.add(rdcUpdate);

				SEndpoint endpoint = null;
				int i = 0;

				while (m_Component != null) {

					try {
						// Wait two seconds for message.
						endpoint = multi.waitForMessage(2 * 1000000);
					} catch (Exception e) {
						// Exception thrown if waitForMessage() returns with an endpoint not on the component which owns the Multiplex.
						break;
					}

					if (endpoint == null) {
						
						SNode node;

						node = m_Endpoint.createMessage("reading");
						node.packString("SomeSensor: This is message #" + i++);
						node.packInt((int) (Math.random() * 1000), "someval");
						node.packInt(34, "somevar");

						final String s = m_Endpoint.emit(node);

						runOnUiThread(new Runnable() {
							public void run() {
								tv.setText(s);
							}
						});

					} else if (endpoint.getEndpointName().equals("rdc_update")) {

						SMessage message = rdcUpdate.receive();
						SNode node = message.getTree();

						final String rdcAddress = node.extractString("rdc_address");
						final boolean arrived = node.extractBoolean("arrived");

						message.delete();

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

						Log.i("SomeConsumer", "RDC Update: " + rdcAddress + " has just " + (arrived ? "arrived" : "left"));

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