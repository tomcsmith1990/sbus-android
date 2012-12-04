/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package uk.ac.cam.tcs40.sbus;

import java.io.IOException;

import android.app.Activity;
import android.util.Log;
import android.widget.TextView;
import android.os.Bundle;


public class HelloJni extends Activity
{
	/** Called when the activity is first created. */
	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);

  		CreateSBUS s = new CreateSBUS(getApplicationContext());
  		
		s.store("SomeSensor.cpt");

		s.store("idl/broker.cpt");
		s.store("idl/builtin.cpt");
		s.store("idl/carpark.cpt");
		s.store("idl/cpt_metadata.idl");
		s.store("idl/cpt_status.idl");
		s.store("idl/demo.cpt");
		s.store("idl/map_constraints.idl");
		s.store("idl/rdc_default.priv");
		s.store("idl/rdc.cpt");
		s.store("idl/rdcacl.cpt");
		s.store("idl/sbus.cpt");
		s.store("idl/slowcar.cpt");
		s.store("idl/speek.cpt");
		s.store("idl/spersist.cpt");
		s.store("idl/spoke.cpt");
		s.store("idl/trafficgen.cpt");
		s.store("idl/universalsink.cpt");
		s.store("idl/universalsource.cpt");
		
        try {
        	Log.i("set permission", "chmod 755 " + s.getDirectory() + "/idl");
			Runtime.getRuntime().exec("chmod 755 " + s.getDirectory() + "/idl");
		} catch (IOException e) {} 
        
		s.store("sbuswrapper"); 

		/* Create a TextView and set its content.
		 * the text is retrieved by calling a native
		 * function.
		 */
		final TextView tv = new TextView(this);
		tv.setText("Some Sensor");
		setContentView(tv);
		
		new Thread() {
			public void run() {
				SComponent scomponent = new SComponent("SomeSensor", "an_instance");
				scomponent.addEndpoint("SomeEpt", "BE8A47EBEB58");
				scomponent.addRDC("192.168.0.11:50123");
				//scomponent.addRDC("10.0.2.2:50123");
				scomponent.start("/data/data/uk.ac.cam.tcs40.sbus.sbus/files/SomeSensor.cpt", -1, true);
				scomponent.setPermission("SomeConsumer", "", true);

				while (true) {
					final String s = scomponent.emit("Hello World", (int) (Math.random() * 1000), 7);

					runOnUiThread(new Runnable() {
						public void run() {
							tv.setText(s);
						}
					});

					try {
						Thread.sleep(3000);
					} catch (InterruptedException e) { }
				}
			}
		}.start();

	}    

}
