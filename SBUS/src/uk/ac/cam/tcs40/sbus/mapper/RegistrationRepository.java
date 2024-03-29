package uk.ac.cam.tcs40.sbus.mapper;

import java.util.LinkedList;
import java.util.List;

public class RegistrationRepository {

	private static List<Registration> s_Registrations = new LinkedList<Registration>();
	
	public static synchronized boolean find(String port) {
		boolean exists = false;
		for (Registration registration : s_Registrations) {
			if (registration.getPort().equals(port)) {
				exists = true;
				break;
			}
		}
		return exists;
	}
	
	public static synchronized Registration find(String component, String instance) {
		if (s_Registrations.size() == 0) return null;
		
		Registration registration = null;
		for (Registration r : s_Registrations) {
			if (r.getComponentName().equals(component) && r.getInstanceName().equals(instance)) {
				registration = r;
				break;
			}
		}
		return registration;
	}
	
	public static synchronized Registration add(String port, String component, String instance) {
		if (find(port) == false) {
			Registration registration = new Registration(port, component, instance);
			s_Registrations.add(registration);
			return registration;
		}
		return null;
	}
	
	public static synchronized Registration remove(String port) {
		for (int i = 0; i < s_Registrations.size(); i++) {
			if (s_Registrations.get(i).getPort().equals(port)) {
				return s_Registrations.remove(i);
			}
		}
		return null;
	}
	
	public static synchronized List<Registration> list() {
		List<Registration> registrations = new LinkedList<Registration>();
		for (Registration r : s_Registrations)
			registrations.add(r);
		return registrations;
	}
	
	public static synchronized Registration getOldest() {
		if (s_Registrations.size() == 0) return null;
		
		Registration registration = s_Registrations.get(0);
		for (Registration r : s_Registrations) {
			if (r.getLastCheckedAlive().before(registration.getLastCheckedAlive())) {
				registration = r;	
			}
		}
		registration.update();
		return registration;
	}
}
