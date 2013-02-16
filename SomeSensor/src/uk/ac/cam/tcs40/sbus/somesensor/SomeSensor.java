package uk.ac.cam.tcs40.sbus.somesensor;

import uk.ac.cam.tcs40.sbus.SComponent;
import uk.ac.cam.tcs40.sbus.SComponent.EndpointType;
import uk.ac.cam.tcs40.sbus.SEndpoint;
import uk.ac.cam.tcs40.sbus.FileBootloader;
import uk.ac.cam.tcs40.sbus.SNode;
import android.app.Activity;
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
				m_Component.setRDCUpdateAutoconnect(true);
				m_Component.start(getApplicationContext().getFilesDir() + "/" + cptFile, -1, true);
				m_Component.setPermission("SomeConsumer", "", true);
				
				m_Endpoint.setAutomapPolicy("+NSomeConsumer", "SomeEpt");
				
				int i = 0;
				SNode node;
				while (m_Component != null) {

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

					try {
						Thread.sleep(2000);
					} catch (InterruptedException e) { }
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