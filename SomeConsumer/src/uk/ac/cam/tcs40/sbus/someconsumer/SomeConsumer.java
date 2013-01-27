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
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.util.Log;
import android.widget.TextView;

public class SomeConsumer extends Activity {

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_some_consumer);

		// Create a FileBootloader to store our component file.
		new FileBootloader(getApplicationContext()).store("SomeConsumer.cpt");

		final TextView someStringTV = (TextView) findViewById(R.id.somestring);
		final TextView someValTV = (TextView) findViewById(R.id.someval);
		final TextView someVarTV = (TextView) findViewById(R.id.somevar);
		
		final Activity activity = this;

		// Create a thread to run the sensor.
		new Thread() {
			public void run() {
				final SComponent scomponent = new SComponent("SomeConsumer", "instance");
				final SEndpoint sendpoint = scomponent.addEndpoint("SomeEpt", EndpointType.EndpointSink, "BE8A47EBEB58");
				// 10.0.2.2 is the development machine when running in AVD.
				//scomponent.addRDC("10.0.2.2:50123");
				final String cptFile = "SomeConsumer.cpt";
				scomponent.start(getApplicationContext().getFilesDir() + "/" + cptFile, 44443, true);
				scomponent.setPermission("SomeSensor", "", true);

				final SEndpoint rdcUpdate = scomponent.RDCUpdateNotificationsEndpoint();

				SMessage message;
				SNode node;
				SEndpoint endpoint;

				final Multiplex multi = scomponent.getMultiplex();
				multi.add(sendpoint);
				multi.add(rdcUpdate);

				while (true) {

					try {
						endpoint = multi.waitForMessage();
					} catch (Exception e) {
						// Exception thrown if waitForMessage() returns with an endpoint not on the component which owns the Multiplex.
						break;
					}

					if (endpoint.getEndpointName().equals("rdc_update")) {

						message = rdcUpdate.receive();
						node = message.getTree();

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
											scomponent.addRDC(rdcAddress);
										else
											scomponent.removeRDC(rdcAddress);
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

					} else if (endpoint.getEndpointName().equals("SomeEpt")) {

						message = sendpoint.receive();
						node = message.getTree();

						final String somestring = node.extractString("somestring");
						final int someval = node.extractInt("someval");
						final int somevar = node.extractInt("somevar");

						message.delete();

						runOnUiThread(new Runnable() {
							public void run() {
								someStringTV.setText("somestring: " + somestring);
								someValTV.setText("someval: " + someval);
								someVarTV.setText("somevar: " + somevar);
							}
						});	
					}
				}
			}
		}.start();

	}
}
