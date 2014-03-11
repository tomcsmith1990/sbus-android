package uk.ac.cam.tcs40.sbus.mapper;

import java.util.List;

import uk.ac.cam.tcs40.sbus.Multiplex;
import uk.ac.cam.tcs40.sbus.Policy.AIRS;
import uk.ac.cam.tcs40.sbus.SComponent;
import uk.ac.cam.tcs40.sbus.SComponent.EndpointType;
import uk.ac.cam.tcs40.sbus.Policy;
import uk.ac.cam.tcs40.sbus.SEndpoint;
import uk.ac.cam.tcs40.sbus.SMessage;
import uk.ac.cam.tcs40.sbus.SNode;
import android.content.Context;

public class PhoneManagementComponent {
	
	private final int m_DefaultRdcPort = 50123;
	public static final String CPT_FILE = "pmc.cpt";

	private static String s_PhoneIP = "127.0.0.1";	// localhost to begin with.
	
	private SComponent m_Component;
	
	private SEndpoint m_Register;
	private SEndpoint m_SetACL;
	private SEndpoint m_Status;
	private SEndpoint m_MapPolicy;
	private SEndpoint m_AIRS;
	
	private static SEndpoint s_Map;
	private static SEndpoint s_List;
	private static SEndpoint s_Lookup;
	private static SEndpoint s_RegisterRdc;
	
	private final Context m_Context;
	
	private AirsEndpointManager m_AirsEndpointManager;
	private PolicyDirectory m_PolicyDirectory;
	private ComponentRegister m_ComponentRegister;
	private ComponentPermissions m_ComponentPermissions;

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
		m_Context = context;
	}

	private void checkAlive() {
		while (m_Component != null) {
			Registration registration = RegistrationRepository.getOldest();
			if (registration != null) {
				String port = registration.getPort();
				String status = m_Status.map(":" + port, "get_status");
				if (status == null) {
					// Have lost contact with this component.
					RegistrationRepository.remove(port);
					PMCActivity.addStatus("Ping indicates component " + registration.getComponentName() + " at :" + port +  
							" vanished without deregistering; removing it from list");
				}
				m_Status.unmap();
			}

			try {
				Thread.sleep(5000);
			} catch (InterruptedException e) {
				e.printStackTrace();
			}
		}
	}

	private void receive() {
		Multiplex multi = m_Component.getMultiplex();
		multi.add(m_Register);
		multi.add(m_SetACL);
		multi.add(m_MapPolicy);
		multi.add(m_AIRS);

		SEndpoint endpoint;
		String name;

		while (m_Component != null) {
			try {
				endpoint = multi.waitForMessage();
			} catch (Exception e) {
				// This means we've added an endpoint which isn't on this component,
				// or that the component has been destroyed by the activity ending.
				continue;
			}
			name = endpoint.getEndpointName();

			if (name.equals("register")) {
				m_ComponentRegister.acceptRegistration(s_PhoneIP);
			} else if (name.equals("set_acl")) {
				m_ComponentPermissions.changePermissions();
			} else if (name.equals("map_policy")) {
				changeMapPolicy();
			} else if (name.equals("lookup_cpt")) {
				// There's some problem with components reading these messages, they're currently not using it.
				lookup();
			} else if (name.equals("AIRS")) {
				SMessage message = m_AIRS.receive();
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
		
		MapPolicy policy;
		try {
			policy = m_PolicyDirectory.readPolicy();
			
		} catch (Exception e) {
			// probably not found a registration.
			return;
		}
		
		if (policy.createPolicy()) {
			final AIRS airsSensor = policy.getSensor();
			final String sensorCode = Policy.sensorCode(airsSensor);
			
			m_AirsEndpointManager.subscribeToAIRS(m_ComponentRegister.getAirsServerAddress(), sensorCode);
		}
	}

	public void start() {
		// Our mapping/rdc component.
		m_Component = new SComponent("rdc", "phone");

		m_ComponentRegister = new ComponentRegister(m_Component);
		m_ComponentPermissions = new ComponentPermissions(m_Component);
		m_PolicyDirectory = new PolicyDirectory(m_Component);
		m_AirsEndpointManager = new AirsEndpointManager(m_Component);
		
		// For components registering to the rdc.
		m_Register = m_ComponentRegister.createRegistrationEndpoint();

		// For components sending permissions after registering.
		m_SetACL = m_ComponentPermissions.createPermissionsEndpoint();

		// Fpr checking components are still alive.
		m_Status = m_Component.addEndpoint("get_status", EndpointType.EndpointClient, "000000000000", "253BAC1C33C7");

		// For mapping components to other components.
		s_Map = m_Component.addEndpoint("map", EndpointType.EndpointSource, "F46B9113DB2D");

		// For getting a list of components to map by name.
		s_List = m_Component.addEndpoint("list", EndpointType.EndpointClient, "000000000000", "46920F3551F9");

		// For telling components to connect to an RDC.
		s_RegisterRdc = m_Component.addEndpoint("register_rdc", EndpointType.EndpointSource, "13ACF49714C5");

		// For any map lookups the component makes.
		s_Lookup = m_Component.addEndpoint("lookup_cpt", EndpointType.EndpointServer, "18D70E4219C8", "F96D2B7A73C1");

		m_MapPolicy = m_PolicyDirectory.addPolicySinkEndpoint();

		m_AIRS = m_AirsEndpointManager.addDataSinkEndpoint();
		
		m_AirsEndpointManager.addSubscriptionEndpoint();

		// Start the component on the default RDC port.
		m_Component.start(m_Context.getFilesDir() + "/" + CPT_FILE, m_DefaultRdcPort, false);

		// Allow all components to connect to endpoints (for register).
		m_Component.setPermission("", "", true);

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
				
		m_ComponentRegister.unmapRegistrationEndpoint();
		
		s_RegisterRdc.unmap();
		m_ComponentPermissions.unmapPermissionsEndpoint();
		m_Status.unmap();
		
		m_PolicyDirectory.unmapPolicySinkEndpoint();		
		m_AirsEndpointManager.unmapDataSinkEndpoint();
		m_AirsEndpointManager.unmapSubscriptionEndpoint();

		m_Component.delete();

		s_Map = null;
		s_List = null;
		s_Lookup = null;
		m_Register = null;
		s_RegisterRdc = null;
		m_SetACL = null;
		m_Status = null;
		m_MapPolicy = null;
		m_AIRS = null;
		m_Component = null;

		s_PhoneIP = "127.0.0.1";
	}

}
