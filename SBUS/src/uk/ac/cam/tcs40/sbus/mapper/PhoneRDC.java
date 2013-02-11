package uk.ac.cam.tcs40.sbus.mapper;

import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.IBinder;
import android.util.Log;

import uk.ac.cam.tcs40.sbus.Multiplex;
import uk.ac.cam.tcs40.sbus.SComponent;
import uk.ac.cam.tcs40.sbus.SComponent.EndpointType;
import uk.ac.cam.tcs40.sbus.SEndpoint;
import uk.ac.cam.tcs40.sbus.SMessage;
import uk.ac.cam.tcs40.sbus.SNode;

public class PhoneRDC extends Service {

	private static PhoneRDC s_PhoneRDC;

	private static final int DEFAULT_RDC_PORT = 50123;

	public static final String RDC_ADDRESS = "192.168.0.3:50123";

	public static final String CPT_FILE = "phonerdc.cpt";

	public static final String TAG = "PhoneRDC";

	private static String s_IP = "127.0.0.1";	// localhost to begin with.

	private static SComponent s_RDCComponent;
	private static SEndpoint s_Register, s_SetACL, s_Status, s_Map, s_List, s_RegisterRdc;

	private static Context s_Context;

	public PhoneRDC() {
		PhoneRDC.s_PhoneRDC = this;
	}

	public PhoneRDC(Context context) {
		PhoneRDC.s_Context = context;
	}

	@Override
	public void onCreate() {
		PhoneRDC.s_Context = getApplicationContext();
		PhoneRDC.s_PhoneRDC.startRDC();
	}

	@Override
	public void onDestroy() {
		PhoneRDC.s_PhoneRDC.stopRDC();
	}

	private static String lookup(String component) {
		/*
		 *  applyMappingPolicies() gets the smessage once.
		 *  It then searches the same message before deleting it.
		 *  This could be useful for mapping components later.
		 */
		if (PhoneRDC.s_List == null) return null;

		// Map the endpoint to the RDC.
		PhoneRDC.s_List.map(PhoneRDC.RDC_ADDRESS, null);
		// Perform an RPC to get the list.
		SMessage reply = PhoneRDC.s_List.rpc(null);

		String address = null;

		if (reply != null)
		{
			address = PhoneRDC.search(reply, component);

			// Delete the native copy of the reply.
			reply.delete();
		}

		// Unmap the endpoint.
		PhoneRDC.s_List.unmap();

		return address;
	}

	private static String search(SMessage reply, String component) {
		SNode snode = reply.getTree();
		SNode item;
		String address = null;

		for (int i = 0; i < snode.count(); i++) {
			item = snode.extractItem(i);
			if (item.extractString("cpt-name").equals(component)) {
				address = item.extractString("address");
				break;
			}
		}
		return address;
	}

	public static void applyMappingPolicies() {
		if (PhoneRDC.s_List == null) return;

		// Map the endpoint to the RDC.
		PhoneRDC.s_List.map(PhoneRDC.RDC_ADDRESS, null);
		/*
		 * Perform an RPC to get the list.
		 * Seeing as these are all being applied in quick succession,
		 * don't really need to get the list of components each time.
		 */
		SMessage reply = PhoneRDC.s_List.rpc(null);

		if (reply != null)
		{
			String address;

			for (Registration registration : RegistrationRepository.list()) {

				for (MapPolicy policy : registration.getMapPolicies()) {
					// Search in the reply for the component by name.
					address = PhoneRDC.search(reply, policy.getPeerComponent());

					// If a component has been found, apply the mapping policy.
					if (address != null)
						PhoneRDC.map(":" + registration.getPort(), policy.getLocalEndpoint(), address, policy.getPeerEndpoint());
				}
			}

			// Delete the native copy of the reply.
			reply.delete();
		}
		// Unmap the endpoint.
		PhoneRDC.s_List.unmap();
	}

	private static void map(String localAddress, String localEndpoint, String targetAddress, String targetEndpoint) {
		if (PhoneRDC.s_Map == null) return;

		if (localAddress == null || localEndpoint == null || targetAddress == null || targetEndpoint == null) return;

		// Send a message to map this component to another.
		PhoneRDC.s_Map.map(localAddress, null);

		SNode node = PhoneRDC.s_Map.createMessage("map");
		node.packString(localEndpoint, "endpoint");
		node.packString(targetAddress, "peer_address");
		node.packString(targetEndpoint, "peer_endpoint");
		node.packString("", "certificate");

		PhoneRDC.s_Map.emit(node);

		PhoneRDC.s_Map.unmap();
	}

	public static void registerRDC(boolean arrived) {
		if (PhoneRDC.s_RegisterRdc == null) return;

		// Inform registered components about the new RDC.
		for (Registration registration : RegistrationRepository.list()) {
			// Send a message to each registered component with the new RDC address.
			PhoneRDC.s_RegisterRdc.map(PhoneRDC.s_IP + ":" + registration.getPort(), "register_rdc");

			SNode node = PhoneRDC.s_RegisterRdc.createMessage("event");
			node.packString(PhoneRDC.RDC_ADDRESS, "rdc_address");
			node.packBoolean(arrived,  "arrived");

			PhoneRDC.s_RegisterRdc.emit(node);
			PhoneRDC.s_RegisterRdc.unmap();
		}

		// Apply mapping policies.
		PhoneRDC.applyMappingPolicies();
	}

	public void startRDC() {
		// Our mapping/rdc component.
		PhoneRDC.s_RDCComponent = new SComponent("rdc", "phone");

		// For components registering to the rdc.
		PhoneRDC.s_Register = PhoneRDC.s_RDCComponent.addEndpoint("register", EndpointType.EndpointSink, "B3572388E4A4");

		// For components sending permissions after registering.
		PhoneRDC.s_SetACL = PhoneRDC.s_RDCComponent.addEndpoint("set_acl", EndpointType.EndpointSink, "6AF2ED96750B");

		// Fpr checking components are still alive.
		PhoneRDC.s_Status = PhoneRDC.s_RDCComponent.addEndpoint("get_status", EndpointType.EndpointClient, "000000000000", "253BAC1C33C7");

		// For mapping components to other components.
		PhoneRDC.s_Map = PhoneRDC.s_RDCComponent.addEndpoint("map", EndpointType.EndpointSource, "F46B9113DB2D");

		// For getting a list of components to map by name.
		PhoneRDC.s_List = PhoneRDC.s_RDCComponent.addEndpoint("list", EndpointType.EndpointClient, "000000000000", "46920F3551F9");

		// For telling components to connect to an RDC.
		PhoneRDC.s_RegisterRdc = PhoneRDC.s_RDCComponent.addEndpoint("register_rdc", EndpointType.EndpointSource, "13ACF49714C5");

		// Start the component on the default RDC port.
		PhoneRDC.s_RDCComponent.start(s_Context.getFilesDir() + "/" + CPT_FILE, DEFAULT_RDC_PORT, false);

		// Allow all components to connect to endpoints (for register).
		PhoneRDC.s_RDCComponent.setPermission("", "", true);

		// Start receiving messages.
		new Thread() {
			public void run() {
				receive();
			}
		}.start();

		// Check components are still alive.
		new Thread() {
			public void run() {
				checkAlive();
			}
		}.start();
	}

	public void stopRDC() {
		PhoneRDC.s_Map.unmap();
		PhoneRDC.s_List.unmap();
		PhoneRDC.s_Register.unmap();
		PhoneRDC.s_RegisterRdc.unmap();
		PhoneRDC.s_SetACL.unmap();
		PhoneRDC.s_Status.unmap();

		PhoneRDC.s_RDCComponent.delete();

		PhoneRDC.s_Map = null;
		PhoneRDC.s_List = null;
		PhoneRDC.s_Register = null;
		PhoneRDC.s_RegisterRdc = null;
		PhoneRDC.s_SetACL = null;
		PhoneRDC.s_Status = null;

		PhoneRDC.s_RDCComponent = null;

		PhoneRDC.s_IP = "127.0.0.1";
	}

	private void receive() {
		Multiplex multi = PhoneRDC.s_RDCComponent.getMultiplex();
		multi.add(PhoneRDC.s_Register);
		multi.add(PhoneRDC.s_SetACL);

		SEndpoint endpoint;
		String name;

		while (PhoneRDC.s_RDCComponent != null) {
			try {
				endpoint = multi.waitForMessage();
			} catch (Exception e) {
				// This means we've added an endpoint which isn't on this component,
				// or that the component has been destroyed by the activity ending.
				continue;
			}
			name = endpoint.getEndpointName();

			if (name.equals("register")) {
				acceptRegistration();
			} else if (name.equals("set_acl")) {
				changePermissions();
			}
		}
	}

	private void acceptRegistration() {
		String address, host, port, sourceComponent, sourceInstance;
		boolean arrived;
		SMessage message;
		SNode snode;

		message = PhoneRDC.s_Register.receive();
		sourceComponent = message.getSourceComponent();
		sourceInstance = message.getSourceInstance();

		snode = message.getTree();
		address = snode.extractString("address");
		arrived = snode.extractBoolean("arrived");

		message.delete();

		host = address.split(":")[0];
		port = address.split(":")[1];

		if (!host.equals(PhoneRDC.s_IP) && !host.equals("127.0.0.1") && !host.equals(""))
			return;

		if (arrived) {

			Registration registration = RegistrationRepository.add(port, sourceComponent, sourceInstance);
			if (registration != null) {
				Log.i(PhoneRDC.TAG, "Registered component " + sourceComponent + " instance " + sourceInstance + ", at :" + port);
			} else {
				Log.i(PhoneRDC.TAG, "Attempting to register already registered component " + sourceComponent + ":" + sourceInstance);
			}

			// TODO: make this automatic.
			if (sourceComponent.equals("SomeSensor"))
				registration.addMapPolicy("SomeEpt", "SomeConsumer", "SomeEpt");

		} else {
			Registration removed = RegistrationRepository.remove(port);
			Log.i(PhoneRDC.TAG, "Deregistered component " + removed.getComponentName() + ":" + removed.getInstanceName() + " at :" + port);
		}
	}

	private void changePermissions() {
		String targetComponent, targetInstance, //targetAddress, targetEndpoint,
		principalComponent, principalInstance, 
		sourceComponent, sourceInstance;
		boolean allow;

		SMessage message;
		SNode snode;

		message = PhoneRDC.s_SetACL.receive();
		sourceComponent = message.getSourceComponent();
		sourceInstance = message.getSourceInstance();

		snode = message.getTree();

		targetComponent = snode.extractString("target_cpt");
		targetInstance = snode.extractString("target_inst");
		//targetAddress = snode.extractString("target_address");
		//targetEndpoint = snode.extractString("target_endpt");
		principalComponent = snode.extractString("principal_cpt");
		principalInstance = snode.extractString("principal_inst");
		allow = snode.extractBoolean("add_perm");

		message.delete();

		if (!targetComponent.equals(sourceComponent) || !targetInstance.equals(sourceInstance))
			return;	// components can only update own permissions.

		if (targetComponent.equals("rdc") || targetComponent.equals("RDC")) {
			// local rule, handled by wrapper.
			PhoneRDC.s_RDCComponent.setPermission(principalComponent, principalInstance, allow);
		} else {
			Registration registration = RegistrationRepository.find(targetComponent, targetInstance);
			if (registration != null) {
				registration.addPermission(principalComponent, principalInstance, allow);
			}
		}
	}

	private void checkAlive() {
		while (PhoneRDC.s_RDCComponent != null) {
			Registration registration = RegistrationRepository.getOldest();
			if (registration != null) {
				String port = registration.getPort();
				String status = PhoneRDC.s_Status.map(":" + port, "get_status");
				if (status == null) {
					RegistrationRepository.remove(port);
					Log.i(PhoneRDC.TAG, "Ping indicates component " + registration.getComponentName() + " at :" + port +  
							" vanished without deregistering; removing it from list");
				}
				PhoneRDC.s_Status.unmap();
			}

			try {
				Thread.sleep(5000);
			} catch (InterruptedException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}
	}

	public static void setIP(String ip) {
		PhoneRDC.s_IP = ip;
	}

	@Override
	public IBinder onBind(Intent arg0) {
		// TODO Auto-generated method stub
		return null;
	}
}