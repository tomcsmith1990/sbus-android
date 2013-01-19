package uk.ac.cam.tcs40.sbus.mapper;

import android.content.Context;
import android.util.Log;

import uk.ac.cam.tcs40.sbus.SComponent;
import uk.ac.cam.tcs40.sbus.SEndpoint;
import uk.ac.cam.tcs40.sbus.SMessage;
import uk.ac.cam.tcs40.sbus.SNode;

public class PhoneRDC {

	private final int DEFAULT_RDC_PORT = 50123;

	public static final String CPT_FILE = "phonerdc.cpt";

	public static final String COMPONENT_ADDR = ":44444";
	public static final String COMPONENT_EPT = "SomeEpt";
	public static final String TARGET_ADDR = "192.168.0.3:44444";
	public static final String TARGET_EPT = "SomeEpt";
	public static final String RDC_ADDRESS = "192.168.0.3:50123";
	public static final String TAG = "PhoneRDC";

	private static SComponent s_RDCComponent;
	private static SEndpoint s_Register, s_SetACL, s_Status, s_Map, s_RegisterRdc;

	private static String s_IP = "127.0.0.1";	// localhost to begin with.
	private static Context s_Context;

	public PhoneRDC(Context context) {
		PhoneRDC.s_Context = context;

		startRDC();

		new Thread() {
			public void run() {
				acceptRegistrations();
			}
		}.start();

		new Thread() {
			public void run() {
				checkAlive();
			}
		}.start();
	}

	public static void remap() {
		if (PhoneRDC.s_Map == null) return;

		// Send a message to map this component to another.
		PhoneRDC.s_Map.map(PhoneRDC.COMPONENT_ADDR, null);

		SNode node = PhoneRDC.s_Map.createMessage("map");
		node.packString(PhoneRDC.COMPONENT_EPT, "endpoint");
		node.packString(PhoneRDC.TARGET_ADDR, "peer_address");
		node.packString(PhoneRDC.TARGET_EPT, "peer_endpoint");
		node.packString("", "certificate");

		PhoneRDC.s_Map.emit(node);

		PhoneRDC.s_Map.unmap();
	}

	public static void registerRDC() {
		if (PhoneRDC.s_RegisterRdc == null) return;

		for (Registration registration : RegistrationRepository.list()) {
			// Send a message to each registered component with the new RDC address.
			PhoneRDC.s_RegisterRdc.map(PhoneRDC.s_IP + ":" + registration.getPort(), "register_rdc");

			SNode node = PhoneRDC.s_RegisterRdc.createMessage("event");
			node.packString(PhoneRDC.RDC_ADDRESS, "rdc_address");

			PhoneRDC.s_RegisterRdc.emit(node);
			PhoneRDC.s_RegisterRdc.unmap();
		}
	}

	private void startRDC() {
		// Our mapping/rdc component.
		PhoneRDC.s_RDCComponent = new SComponent("rdc", "rdc");

		// For components registering to the rdc.
		PhoneRDC.s_Register = PhoneRDC.s_RDCComponent.addEndpointSink("register", "B3572388E4A4");

		// For components sending permissions after registering.
		PhoneRDC.s_SetACL = PhoneRDC.s_RDCComponent.addEndpointSink("set_acl", "6AF2ED96750B");

		// Fpr checking components are still alive.
		PhoneRDC.s_Status = PhoneRDC.s_RDCComponent.addEndpointClient("get_status", "000000000000", "253BAC1C33C7");

		// For mapping components to other components.
		PhoneRDC.s_Map = PhoneRDC.s_RDCComponent.addEndpointSource("map", "F46B9113DB2D");

		// For telling components to connect to an rdc.
		PhoneRDC.s_RegisterRdc = PhoneRDC.s_RDCComponent.addEndpointSource("register_rdc", "3D3F1711E783");

		// Start the component on the default rdc port.
		PhoneRDC.s_RDCComponent.start(s_Context.getFilesDir() + "/" + CPT_FILE, DEFAULT_RDC_PORT, false);

		// Allow all components to connect to endpoints (for register).
		PhoneRDC.s_RDCComponent.setPermission("", "", true);
	}

	private void acceptRegistrations() {
		String address, host, port;
		boolean arrived;
		SMessage message;
		SNode snode;

		while (true) {
			message = PhoneRDC.s_Register.receive();
			snode = message.getTree();
			address = snode.extractString("address");
			arrived = snode.extractBoolean("arrived");

			host = address.split(":")[0];
			port = address.split(":")[1];

			if (!host.equals(PhoneRDC.s_IP))
				continue;

			if (arrived) {
				if (RegistrationRepository.add(port)) {
					Log.i(PhoneRDC.TAG, "Registered component " + port);
				} else {
					Log.i(PhoneRDC.TAG, "Attempting to register already registered component.");
				}
			} else {
				RegistrationRepository.remove(port);
				Log.i(PhoneRDC.TAG, "Deregistered component " + port);
			}

			message.delete();
		}
	}

	private void checkAlive() {
		while (true) {
			Registration registration = RegistrationRepository.getOldest();
			if (registration != null) {
				String port = registration.getPort();
				String status = PhoneRDC.s_Status.map(":" + port, "get_status");
				if (status == null) {
					RegistrationRepository.remove(port);
					Log.i(PhoneRDC.TAG, "Ping indicates component :" + port + " vanished without deregistering; removing it from list");
				}
				PhoneRDC.s_Status.unmap();
			}

			try {
				Thread.sleep(1000);
			} catch (InterruptedException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}
	}

	public static void setIP(String ip) {
		PhoneRDC.s_IP = ip;
	}
}