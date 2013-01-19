package uk.ac.cam.tcs40.sbus.mapper;

import java.util.LinkedList;
import java.util.List;

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
	private static SEndpoint s_Register;
	private static SEndpoint s_SetACL;
	private static MapEndpoint s_Map;
	private static RdcEndpoint s_RegisterRdc;

	private static final List<String> s_RegisteredComponents = new LinkedList<String>();
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
	}

	public static void remap() {
		if (PhoneRDC.s_Map == null) return;

		PhoneRDC.s_Map.map(PhoneRDC.COMPONENT_ADDR, PhoneRDC.COMPONENT_EPT, PhoneRDC.TARGET_ADDR, PhoneRDC.TARGET_EPT);
	}

	public static void registerRDC() {
		if (PhoneRDC.s_RegisterRdc == null) return;

		for (String localAddress : PhoneRDC.s_RegisteredComponents)
			PhoneRDC.s_RegisterRdc.registerRdc(PhoneRDC.s_IP + ":" + localAddress, PhoneRDC.RDC_ADDRESS);
	}

	private void startRDC() {
		// Our mapping/rdc component.
		PhoneRDC.s_RDCComponent = new SComponent("rdc", "rdc");

		// For components registering to the rdc.
		PhoneRDC.s_Register = PhoneRDC.s_RDCComponent.addEndpointSink("register", "B3572388E4A4");

		// For components sending permissions after registering.
		PhoneRDC.s_SetACL = PhoneRDC.s_RDCComponent.addEndpointSink("set_acl", "6AF2ED96750B");

		// For mapping components to other components.
		SEndpoint map = PhoneRDC.s_RDCComponent.addEndpointSource("map", "F46B9113DB2D");
		PhoneRDC.s_Map = new MapEndpoint(map);

		// For telling components to connect to an rdc.
		SEndpoint rdc = PhoneRDC.s_RDCComponent.addEndpointSource("register_rdc", "3D3F1711E783");
		PhoneRDC.s_RegisterRdc = new RdcEndpoint(rdc);

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
				if (PhoneRDC.s_RegisteredComponents.contains(port)) {
					Log.i(PhoneRDC.TAG, "Attempting to register already registered component.");
				} else {
					PhoneRDC.s_RegisteredComponents.add(port);
					Log.i(PhoneRDC.TAG, "Registered component " + port);
				}
			} else {
				PhoneRDC.s_RegisteredComponents.remove(port);
				Log.i(PhoneRDC.TAG, "Deregistered component " + port);
			}

			message.delete();
		}
	}
	
	public static void setIP(String ip) {
		PhoneRDC.s_IP = ip;
	}
}