package uk.ac.cam.tcs40.sbus.mapper;

import android.content.Context;
import android.util.Log;
import uk.ac.cam.tcs40.sbus.Multiplex;
import uk.ac.cam.tcs40.sbus.SComponent;
import uk.ac.cam.tcs40.sbus.SEndpoint;
import uk.ac.cam.tcs40.sbus.SMessage;
import uk.ac.cam.tcs40.sbus.SNode;
import uk.ac.cam.tcs40.sbus.SComponent.EndpointType;

public class PhoneRDC {

	private static final int DEFAULT_RDC_PORT = 50123;

	public static final String RDC_ADDRESS = "192.168.0.3:50123";

	public static final String CPT_FILE = "phonerdc.cpt";

	public static final String TAG = "PhoneRDC";

	private static String s_IP = "127.0.0.1";	// localhost to begin with.

	private static SComponent s_RDCComponent;
	private static SEndpoint s_Register, s_SetACL, s_Status, s_Map, s_List, s_Lookup, s_RegisterRdc;
	
	private static Context s_Context;
	
	public PhoneRDC(Context context) {
		s_Context = context;
	}
	
	private static String lookup(String component) {
		/*
		 *  applyMappingPolicies() gets the smessage once.
		 *  It then searches the same message before deleting it.
		 *  This could be useful for mapping components later.
		 */
		if (s_List == null) return null;

		// Map the endpoint to the RDC.
		s_List.map(PhoneRDCActivity.getRDCAddress(), null);
		// Perform an RPC to get the list.
		SMessage reply = s_List.rpc(null);

		String address = null;

		if (reply != null)
		{
			address = search(reply, component);

			// Delete the native copy of the reply.
			reply.delete();
		}

		// Unmap the endpoint.
		s_List.unmap();

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

	public static boolean applyMappingPolicies() {
		if (s_List == null) return false;

		// Map the endpoint to the RDC.
		s_List.map(PhoneRDCActivity.getRDCAddress(), null);
		/*
		 * Perform an RPC to get the list.
		 * Seeing as these are all being applied in quick succession,
		 * don't really need to get the list of components each time.
		 */
		SMessage reply = s_List.rpc(null);

		if (reply == null) {
			// Unmap the endpoint.
			s_List.unmap();
			return false;
		}

		String address;

		for (Registration registration : RegistrationRepository.list()) {

			for (MapPolicy policy : registration.getMapPolicies()) {
				// Search in the reply for the component by name.
				address = search(reply, policy.getPeerComponent());

				// If a component has been found, apply the mapping policy.
				if (address != null)
					map(":" + registration.getPort(), policy.getLocalEndpoint(), address, policy.getPeerEndpoint());
			}
		}

		// Delete the native copy of the reply.
		reply.delete();
		// Unmap the endpoint.
		s_List.unmap();

		return true;
	}

	private static void map(String localAddress, String localEndpoint, String targetAddress, String targetEndpoint) {
		if (s_Map == null) return;

		if (localAddress == null || localEndpoint == null || targetAddress == null || targetEndpoint == null) return;

		// Send a message to map this component to another.
		s_Map.map(localAddress, null);

		SNode node = s_Map.createMessage("map");
		node.packString(localEndpoint, "endpoint");
		node.packString(targetAddress, "peer_address");
		node.packString(targetEndpoint, "peer_endpoint");
		node.packString("", "certificate");

		s_Map.emit(node);

		s_Map.unmap();
	}

	public static void registerRDC(boolean arrived) {
		if (s_RegisterRdc == null) return;

		// Apply mapping policies. Returns false if there is no RDC.
		// Doing this first means we won't map components on the phone together due to race conditions in registering.
		if (applyMappingPolicies()) {

			// Inform registered components about the new RDC.
			for (Registration registration : RegistrationRepository.list()) {
				// Send a message to each registered component with the new RDC address.
				s_RegisterRdc.map(s_IP + ":" + registration.getPort(), "register_rdc");

				SNode node = s_RegisterRdc.createMessage("event");
				node.packString(PhoneRDCActivity.getRDCAddress(), "rdc_address");
				node.packBoolean(arrived,  "arrived");

				s_RegisterRdc.emit(node);
				s_RegisterRdc.unmap();
			}
		}
	}

	public void startRDC() {
		// Our mapping/rdc component.
		s_RDCComponent = new SComponent("rdc", "phone");

		// For components registering to the rdc.
		s_Register = s_RDCComponent.addEndpoint("register", EndpointType.EndpointSink, "B3572388E4A4");

		// For components sending permissions after registering.
		s_SetACL = s_RDCComponent.addEndpoint("set_acl", EndpointType.EndpointSink, "6AF2ED96750B");

		// Fpr checking components are still alive.
		s_Status = s_RDCComponent.addEndpoint("get_status", EndpointType.EndpointClient, "000000000000", "253BAC1C33C7");

		// For mapping components to other components.
		s_Map = s_RDCComponent.addEndpoint("map", EndpointType.EndpointSource, "F46B9113DB2D");

		// For getting a list of components to map by name.
		s_List = s_RDCComponent.addEndpoint("list", EndpointType.EndpointClient, "000000000000", "46920F3551F9");

		// For telling components to connect to an RDC.
		s_RegisterRdc = s_RDCComponent.addEndpoint("register_rdc", EndpointType.EndpointSource, "13ACF49714C5");
		
		// For any map lookups the component makes.
		s_Lookup = s_RDCComponent.addEndpoint("lookup_cpt", EndpointType.EndpointServer, "AE7945554959", "6AA2406BF9EC");

		// Start the component on the default RDC port.
		s_RDCComponent.start(s_Context.getFilesDir() + "/" + CPT_FILE, DEFAULT_RDC_PORT, false);

		// Allow all components to connect to endpoints (for register).
		s_RDCComponent.setPermission("", "", true);

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
		s_Map.unmap();
		s_List.unmap();
		s_Lookup.unmap();
		s_Register.unmap();
		s_RegisterRdc.unmap();
		s_SetACL.unmap();
		s_Status.unmap();

		s_RDCComponent.delete();

		s_Map = null;
		s_List = null;
		s_Lookup = null;
		s_Register = null;
		s_RegisterRdc = null;
		s_SetACL = null;
		s_Status = null;

		s_RDCComponent = null;

		s_IP = "127.0.0.1";
	}

	private void receive() {
		Multiplex multi = s_RDCComponent.getMultiplex();
		multi.add(s_Register);
		multi.add(s_SetACL);
		multi.add(s_Lookup);

		SEndpoint endpoint;
		String name;

		while (s_RDCComponent != null) {
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
			} else if (name.equals("lookup_cpt")) {
				SMessage query = s_Lookup.receive();
				Registration r = RegistrationRepository.find(query.getSourceComponent(), query.getSourceInstance());
				if (r != null) {
					// TODO: correct mapping constraints.
						r.addMapPolicy("SomeEpt", "SomeConsumer", "SomeEpt");
				}
				SNode result = s_Lookup.createMessage("results");
				s_Lookup.reply(query, result);
				query.delete();
			}
		}
	}

	private void acceptRegistration() {
		String address, host, port, sourceComponent, sourceInstance;
		boolean arrived;
		SMessage message;
		SNode snode;

		message = s_Register.receive();
		sourceComponent = message.getSourceComponent();
		sourceInstance = message.getSourceInstance();

		snode = message.getTree();
		address = snode.extractString("address");
		arrived = snode.extractBoolean("arrived");

		message.delete();

		host = address.split(":")[0];
		port = address.split(":")[1];

		if (!host.equals(s_IP) && !host.equals("127.0.0.1") && !host.equals("") && !host.equals("10.0.2.15"))
			return;

		if (arrived) {

			Registration registration = RegistrationRepository.add(port, sourceComponent, sourceInstance);
			if (registration != null) {
				Log.i(TAG, "Registered component " + sourceComponent + " instance " + sourceInstance + ", at :" + port);
			} else {
				Log.i(TAG, "Attempting to register already registered component " + sourceComponent + ":" + sourceInstance);
			}

		} else {
			Registration removed = RegistrationRepository.remove(port);
			Log.i(TAG, "Deregistered component " + removed.getComponentName() + ":" + removed.getInstanceName() + " at :" + port);
		}
	}

	private void changePermissions() {
		String targetComponent, targetInstance, //targetAddress, targetEndpoint,
		principalComponent, principalInstance, 
		sourceComponent, sourceInstance;
		boolean allow;

		SMessage message;
		SNode snode;

		message = s_SetACL.receive();
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
			s_RDCComponent.setPermission(principalComponent, principalInstance, allow);
		} else {
			Registration registration = RegistrationRepository.find(targetComponent, targetInstance);
			if (registration != null) {
				registration.addPermission(principalComponent, principalInstance, allow);
			}
		}
	}

	private void checkAlive() {
		while (s_RDCComponent != null) {
			Registration registration = RegistrationRepository.getOldest();
			if (registration != null) {
				String port = registration.getPort();
				String status = s_Status.map(":" + port, "get_status");
				if (status == null) {
					RegistrationRepository.remove(port);
					Log.i(TAG, "Ping indicates component " + registration.getComponentName() + " at :" + port +  
							" vanished without deregistering; removing it from list");
				}
				s_Status.unmap();
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
		s_IP = ip;
	}

}
