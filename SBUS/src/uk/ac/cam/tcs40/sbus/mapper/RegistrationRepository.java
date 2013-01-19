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
	
	public static synchronized boolean add(String port) {
		if (find(port) == false) {
			s_Registrations.add(new Registration(port));
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
}
