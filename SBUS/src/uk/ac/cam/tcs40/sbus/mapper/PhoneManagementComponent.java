package uk.ac.cam.tcs40.sbus.mapper;

import java.util.LinkedList;
import java.util.List;

import uk.ac.cam.tcs40.sbus.Multiplex;
import uk.ac.cam.tcs40.sbus.Policy.AIRS;
import uk.ac.cam.tcs40.sbus.Policy.Condition;
import uk.ac.cam.tcs40.sbus.SComponent;
import uk.ac.cam.tcs40.sbus.SComponent.EndpointType;
import uk.ac.cam.tcs40.sbus.Policy;
import uk.ac.cam.tcs40.sbus.SEndpoint;
import uk.ac.cam.tcs40.sbus.SMessage;
import uk.ac.cam.tcs40.sbus.SNode;
import android.content.Context;

public class PhoneManagementComponent {
	
	private static final int DEFAULT_RDC_PORT = 50123;
	public static final String CPT_FILE = "pmc.cpt";

	private static String s_PhoneIP = "127.0.0.1";	// localhost to begin with.
	private static SComponent s_PMC;
	private static SEndpoint s_Register, s_SetACL, s_Status, s_Map, s_List, s_Lookup, 
	s_RegisterRdc, s_MapPolicy, s_AIRS, s_AIRSSubscribe;

	private static String s_AIRSAddress;
	private static final List<String> s_AIRSSubscriptions = new LinkedList<String>();

	private static Context s_Context;

	/**
	 * Apply any mapping policies which any of the registered components have sent us.
	 * @return true if the current RDC exists.
	 */
	public static void applyMappingPolicies() {
		applyMappingPolicies(null, 0);
	}

	public static void applyMappingPolicies(String sensorCode, int value) {
		
		if (s_List == null) return;

		for (Registration registration : RegistrationRepository.list()) {

			for (MapPolicy policy : registration.getMapPolicies()) {
				// Tell each component to map() as it has specified.
				boolean map;

				if (sensorCode == null || policy.getSensor() == AIRS.NONE)
					map = true;
				else {
					String policyCode = Policy.sensorCode(policy.getSensor());
					if (!policyCode.equals(sensorCode)) {
						// wrong sensor event
						continue;
					}

					switch (policy.getCondition()) {
					case NONE:
						map = true;
						break;
					case EQUAL:
						map = (value == policy.getValue());
						break;
					case GREATER_THAN:
						map = (value > policy.getValue());
						break;
					case GREATER_THAN_EQUAL:
						map = (value >= policy.getValue());
						break;
					case LESS_THAN:
						map = (value < policy.getValue());
						break;
					case LESS_THAN_EQUAL:
						map = (value <= policy.getValue());
						break;
					default:
						map = false;
						break;
					}
				}

				if (map)
					map(":" + registration.getPort(), policy.getLocalEndpoint(), policy.getRemoteAddress(), policy.getRemoteEndpoint());
			}
		}
	}

	public static void applyMappingPoliciesLocally() {
		List<Registration> localComponents = RegistrationRepository.list();
		Registration mapFrom, mapTo;
		for (int i = 0; i < localComponents.size(); i++) {
			mapFrom = localComponents.get(i);
			for (MapPolicy policy : mapFrom.getMapPolicies()) {
				for (int j = 0; j < localComponents.size(); j++) {
					mapTo = localComponents.get(j);
					if (new MapConstraint(policy.getRemoteAddress()).match(mapTo)) {
						map(":" + mapFrom.getPort(), policy.getLocalEndpoint(), ":" + mapTo.getPort(), policy.getRemoteEndpoint());
						break;
					}
				}
			}
		}
	}

	/**
	 * 
	 * @return The current RDC address which we know about.
	 */
	private static String getRemoteRDCAddress() {
		return PMCActivity.getRemoteRDCAddress();
	}

	/**
	 * Tell an endpoint to map to another endpoint.
	 * @param localAddress The address of the component to perform the map.
	 * @param localEndpoint The endpoint to perform the map.
	 * @param remoteAddress The address of the component to map to.
	 * @param remoteEndpoint The endpoint to map to.
	 */
	private static void map(String localAddress, String localEndpoint, String remoteAddress, String remoteEndpoint) {
		if (s_Map == null) return;

		if (localAddress == null || localEndpoint == null || remoteAddress == null || remoteEndpoint == null) return;

		// Send a message to map this component to another.
		s_Map.map(localAddress, null);

		SNode node = s_Map.createMessage("map");
		node.packString(localEndpoint, "endpoint");
		node.packString(remoteAddress, "peer_address");
		node.packString(remoteEndpoint, "peer_endpoint");
		node.packString("", "certificate");

		s_Map.emit(node);

		s_Map.unmap();
	}

	/**
	 * Inform any registered components that we've found/lost an RDC. If found, apply any mapping policies we know about.
	 * @param register Components can register with this RDC.
	 */

	public static void informComponentsAboutRDC(boolean register) {
		if (s_RegisterRdc == null || s_List == null) return;

		// Don't do anything if there are no components.
		if (RegistrationRepository.list().size() == 0) return;

		String mapString = s_List.map(getRemoteRDCAddress(), null);

		// RDC doesn't exist.
		if (mapString == null)
			return;

		s_List.unmap();

		// Inform registered components about the new RDC.
		for (Registration registration : RegistrationRepository.list()) {
			// Send a message to each registered component with the RDC address.
			s_RegisterRdc.map(":" + registration.getPort(), "register_rdc");

			SNode node = s_RegisterRdc.createMessage("event");
			node.packString(getRemoteRDCAddress(), "rdc_address");
			node.packBoolean(register,  "arrived");

			s_RegisterRdc.emit(node);
			s_RegisterRdc.unmap();
		}
	}

	/**
	 * Update the phone's IP.
	 * @param ip The phone's IP.
	 */
	public static void setPhoneIP(String ip) {
		s_PhoneIP = ip;
	}

	public PhoneManagementComponent(Context context) {
		s_Context = context;
	}

	private void subscribeToAIRS(String sensorCode) {
		if (s_AIRSAddress == null || sensorCode == null) 
			return;

		if (!s_AIRSSubscriptions.contains(sensorCode)) {
			s_AIRSSubscribe.map(s_AIRSAddress, "subscribe");
			SNode subscription = s_AIRSSubscribe.createMessage("subscription");
			subscription.packString(sensorCode, "sensor");
			s_AIRSSubscribe.emit(subscription);
			s_AIRSSubscribe.unmap();

			s_AIRSSubscriptions.add(sensorCode);
		}

	}

	private void acceptRegistration() {
		String address, host, port, sourceComponent, sourceInstance;
		boolean register;
		SMessage message;
		SNode snode;

		message = s_Register.receive();
		sourceComponent = message.getSourceComponent();
		sourceInstance = message.getSourceInstance();

		snode = message.getTree();
		address = snode.extractString("address");
		register = snode.extractBoolean("arrived");

		message.delete();

		host = address.split(":")[0];
		port = address.split(":")[1];

		// Check this is a component on the phone.
		if (!host.equals(s_PhoneIP) && !host.equals("127.0.0.1") && !host.equals("") && !host.equals("10.0.2.15"))
			return;

		if (register) {

			if (sourceComponent.equals("AirsSensor")) {
				s_AIRSAddress = ":" + port;
				PMCActivity.addStatus("Found AIRS at :" + port);
				return;
				//subscribeToAIRS("WC");
			}

			Registration registration = RegistrationRepository.add(port, sourceComponent, sourceInstance);
			if (registration != null) {
				PMCActivity.addStatus("Registered component " + sourceComponent + " instance " + sourceInstance + ", at :" + port);
			} else {
				PMCActivity.addStatus("Attempting to register already registered component " + sourceComponent + ":" + sourceInstance);
			}

		} else {
			Registration deregistration = RegistrationRepository.remove(port);
			PMCActivity.addStatus("Deregistered component " + deregistration.getComponentName() + ":" + deregistration.getInstanceName() + " at :" + port);
		}
	}

	private void changePermissions() {
		String targetComponent, targetInstance, //targetAddress, targetEndpoint,
		remoteComponent, remoteInstance, 
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
		remoteComponent = snode.extractString("principal_cpt");
		remoteInstance = snode.extractString("principal_inst");
		allow = snode.extractBoolean("add_perm");

		message.delete();

		// Check that the component is setting a permission for itself.
		if (!targetComponent.equals(sourceComponent) || !targetInstance.equals(sourceInstance))
			return;

		if (targetComponent.equals("rdc")) {
			// local rule, handled by wrapper.
			s_PMC.setPermission(remoteComponent, remoteInstance, allow);
		} else {
			Registration registration = RegistrationRepository.find(targetComponent, targetInstance);
			if (registration != null) {
				registration.addPermission(remoteComponent, remoteInstance, allow);
			}
		}
	}

	private void checkAlive() {
		while (s_PMC != null) {
			Registration registration = RegistrationRepository.getOldest();
			if (registration != null) {
				String port = registration.getPort();
				String status = s_Status.map(":" + port, "get_status");
				if (status == null) {
					// Have lost contact with this component.
					RegistrationRepository.remove(port);
					PMCActivity.addStatus("Ping indicates component " + registration.getComponentName() + " at :" + port +  
							" vanished without deregistering; removing it from list");
				}
				s_Status.unmap();
			}

			try {
				Thread.sleep(5000);
			} catch (InterruptedException e) {
				e.printStackTrace();
			}
		}
	}

	private void receive() {
		Multiplex multi = s_PMC.getMultiplex();
		multi.add(s_Register);
		multi.add(s_SetACL);
		multi.add(s_MapPolicy);
		multi.add(s_AIRS);

		SEndpoint endpoint;
		String name;

		while (s_PMC != null) {
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
			} else if (name.equals("map_policy")) {
				changeMapPolicy();
			} else if (name.equals("lookup_cpt")) {
				// There's some problem with components reading these messages, they're currently not using it.
				lookup();
			} else if (name.equals("AIRS")) {
				SMessage message = s_AIRS.receive();
				SNode tree = message.getTree();

				String sensorCode = tree.extractString("sensor");

				if (tree.exists("var")) {
					int value = tree.extractInt("var");

					if (sensorCode.equals("WC"))
						informComponentsAboutRDC(value == 1);

					applyMappingPolicies(sensorCode, value);
				}

				message.delete();
			}
		}
	}

	private void lookup() {
		SMessage query = s_Lookup.receive();
		//SNode constraints = query.getTree().extractItem("map-constraints");
		//SNode interface = query.getTree().extractItem("interface");
		SNode results = s_Lookup.createMessage("results");
		s_Lookup.reply(query, results);
		query.delete();
	}

	private void changeMapPolicy() {
		SMessage message = s_MapPolicy.receive();
		SNode snode = message.getTree();

		boolean create = snode.extractBoolean("create");
		String remoteAddress = snode.extractString("peer_address");
		String remoteEndpoint = snode.extractString("peer_endpoint");
		String localEndpoint = snode.extractString("endpoint");
		int sensor = snode.extractInt("sensor");
		int condition = snode.extractInt("condition");
		int value = snode.extractInt("value");

		MapPolicy policy = new MapPolicy(localEndpoint, remoteAddress, remoteEndpoint, 
				AIRS.values()[sensor], Condition.values()[condition], value);

		Registration registration = RegistrationRepository.find(message.getSourceComponent(), message.getSourceInstance());
		if (registration != null) {

			if (create) {
				registration.addMapPolicy(policy);
				subscribeToAIRS(Policy.sensorCode(AIRS.values()[sensor]));
				PMCActivity.addStatus("Adding map policy: " + remoteAddress + " for component " + registration.getComponentName());
			}
			else {
				registration.removeMapPolicy(policy);
				PMCActivity.addStatus("Removing map policy: " + remoteAddress + " for component " + registration.getComponentName());
			}
		}
		message.delete();

		//subscribeToAIRS("Rd");
	}

	public void start() {
		// Our mapping/rdc component.
		s_PMC = new SComponent("rdc", "phone");

		// For components registering to the rdc.
		s_Register = s_PMC.addEndpoint("register", EndpointType.EndpointSink, "B3572388E4A4");

		// For components sending permissions after registering.
		s_SetACL = s_PMC.addEndpoint("set_acl", EndpointType.EndpointSink, "6AF2ED96750B");

		// Fpr checking components are still alive.
		s_Status = s_PMC.addEndpoint("get_status", EndpointType.EndpointClient, "000000000000", "253BAC1C33C7");

		// For mapping components to other components.
		s_Map = s_PMC.addEndpoint("map", EndpointType.EndpointSource, "F46B9113DB2D");

		// For getting a list of components to map by name.
		s_List = s_PMC.addEndpoint("list", EndpointType.EndpointClient, "000000000000", "46920F3551F9");

		// For telling components to connect to an RDC.
		s_RegisterRdc = s_PMC.addEndpoint("register_rdc", EndpointType.EndpointSource, "13ACF49714C5");

		// For any map lookups the component makes.
		s_Lookup = s_PMC.addEndpoint("lookup_cpt", EndpointType.EndpointServer, "18D70E4219C8", "F96D2B7A73C1");

		s_MapPolicy = s_PMC.addEndpoint("map_policy", EndpointType.EndpointSink, "157EC474FA55");

		s_AIRS = s_PMC.addEndpoint("AIRS", EndpointType.EndpointSink, "6187707D4CCE");

		s_AIRSSubscribe = s_PMC.addEndpoint("airs_subscribe", EndpointType.EndpointSource, "F03F918E91A3");

		// Start the component on the default RDC port.
		s_PMC.start(s_Context.getFilesDir() + "/" + CPT_FILE, DEFAULT_RDC_PORT, false);

		// Allow all components to connect to endpoints (for register).
		s_PMC.setPermission("", "", true);

		// Start receiving messages.
		new Thread() {
			@Override
			public void run() {
				receive();
			}
		}.start();

		// Check components are still alive.
		new Thread() {
			@Override
			public void run() {
				checkAlive();
			}
		}.start();
	}

	public void stop() {
		s_Map.unmap();
		s_List.unmap();
		s_Lookup.unmap();
		s_Register.unmap();
		s_RegisterRdc.unmap();
		s_SetACL.unmap();
		s_Status.unmap();
		s_MapPolicy.unmap();
		s_AIRS.unmap();
		s_AIRSSubscribe.unmap();

		s_PMC.delete();

		s_Map = null;
		s_List = null;
		s_Lookup = null;
		s_Register = null;
		s_RegisterRdc = null;
		s_SetACL = null;
		s_Status = null;
		s_MapPolicy = null;
		s_AIRS = null;
		s_AIRSSubscribe = null;

		s_PMC = null;

		s_PhoneIP = "127.0.0.1";
	}

}
