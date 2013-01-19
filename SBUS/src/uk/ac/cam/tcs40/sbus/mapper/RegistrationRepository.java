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
	
	public static synchronized boolean add(String port, String component, String instance) {
		if (find(port) == false) {
			s_Registrations.add(new Registration(port, component, instance));
			return true;
		}
		return false;
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
			if (r.getDate().before(registration.getDate())) {
				registration = r;	
			}
		}
		registration.update();
		return registration;
	}
}
