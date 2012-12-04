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

import android.app.Activity;
import android.util.Log;
import android.widget.TextView;
import android.os.Bundle;


public class SomeSensor extends Activity
{
	/** Called when the activity is first created. */
	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);

		/*
		 * 
		 * SBUSBootloader will be a separate application, copying this files.
		 * This means that the application context will be different, different directories.
		 * We'll store the .cpt file in the actual application.
		 * 
		 */
		
		// Create the SBUSBootloader.
		// This will copy all necessary files across, then store SomeSensor.cpt.
  		new SBUSBootloader(getApplicationContext());
  		
  		// Create a FileBootloader to store our component file.
  		new FileBootloader(getApplicationContext()).store("SomeSensor.cpt");

  		// Add a TextView to the Activity.
		final TextView tv = new TextView(this);
		tv.setText("Some Sensor");
		setContentView(tv);
		
		// Create a thread to run the sensor.
		new Thread() {
			public void run() {
				SComponent scomponent = new SComponent("SomeSensor", "an_instance");
				scomponent.addEndpoint("SomeEpt", "BE8A47EBEB58");
				scomponent.addRDC("192.168.0.11:50123");
				// 10.0.2.2 is the development machine when running in AVD.
				//scomponent.addRDC("10.0.2.2:50123");
				String cptFile = "SomeSensor.cpt";
				scomponent.start(getApplicationContext().getFilesDir() + "/" + cptFile, -1, true);
				scomponent.setPermission("SomeConsumer", "", true);

				while (true) {
					final String s = scomponent.emit("Hello World", (int) (Math.random() * 1000), 5);

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
