package uk.ac.cam.tcs40.sbus.someconsumer;

import uk.ac.cam.tcs40.sbus.FileBootloader;
import uk.ac.cam.tcs40.sbus.SComponent;
import uk.ac.cam.tcs40.sbus.SEndpoint;
import uk.ac.cam.tcs40.sbus.SMessage;
import uk.ac.cam.tcs40.sbus.SNode;
import android.os.Bundle;
import android.app.Activity;
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

		// Create a thread to run the sensor.
		new Thread() {
			public void run() {
				SComponent scomponent = new SComponent("SomeConsumer", "instance");
				SEndpoint sendpoint = scomponent.addEndpoint("SomeEpt", false, "BE8A47EBEB58");
				scomponent.addRDC("192.168.0.3:50123");
				// 10.0.2.2 is the development machine when running in AVD.
				//scomponent.addRDC("10.0.2.2:50123");
				String cptFile = "SomeConsumer.cpt";
				scomponent.start(getApplicationContext().getFilesDir() + "/" + cptFile, 44443, true);
				scomponent.setPermission("SomeSensor", "", true);

				SMessage message;
				SNode node;
				
				while (true) {

					message = sendpoint.receive();
					node = message.getTree();
					
					final String somestring = node.extractString("somestring");
					final int someval = node.extractInt("someval");
					final int somevar = node.extractInt("somevar");

					runOnUiThread(new Runnable() {
						public void run() {
							someStringTV.setText("somestring: " + somestring);
							someValTV.setText("someval: " + someval);
							someVarTV.setText("somevar: " + somevar);
						}
					});
					
					message.delete();

					try {
						Thread.sleep(2000);
					} catch (InterruptedException e) { }
				}
			}
		}.start();

	}
}
