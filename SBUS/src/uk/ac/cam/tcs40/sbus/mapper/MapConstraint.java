package uk.ac.cam.tcs40.sbus.mapper;

import java.util.LinkedList;
import java.util.List;

import uk.ac.cam.tcs40.sbus.SNode;

public class MapConstraint {

	private String m_ComponentName, m_InstanceName, m_Creator, m_PubKey;
	private List<String> m_Keywords, m_Peers, m_Ancestors;

	public MapConstraint(String constraintString) {
		this.m_Keywords = new LinkedList<String>();
		this.m_Peers = new LinkedList<String>();
		this.m_Ancestors = new LinkedList<String>();

		char code;
		String criteria;

		for (String constraint : constraintString.split("\\+")) {
			if (constraint.equals(""))
				continue;
			
			code = constraint.charAt(0);
			criteria = constraint.substring(1);

			switch (code) {
			case 'N':
				this.m_ComponentName = criteria;
				break;
			case 'I':
				this.m_InstanceName = criteria;
				break;
			case 'U':
				this.m_Creator = criteria;
				break;
			case 'X':
				this.m_PubKey = criteria;
				break;
			case 'K':
				this.m_Keywords.add(criteria);
				break;
			case 'P':
				this.m_Peers.add(criteria);
				break;
			case 'A':
				this.m_Ancestors.add(criteria);
				break;
			default:
				break;
			}	
		}
	}

	public boolean match(SNode possibility) {
		// TODO: get other fields. Using the list endpoint of RDC only gives cpt-name and instance.
		
		if (possibility.exists("cpt-name") && this.m_ComponentName != null && possibility.extractString("cpt-name").equals(this.m_ComponentName) == false)
			return false;

		if (possibility.exists("instance") && this.m_InstanceName != null && possibility.extractString("instance").equals(this.m_InstanceName) == false)
			return false;

		if (possibility.exists("creator") && this.m_Creator != null && possibility.extractString("creator").equals(this.m_Creator) == false)
			return false;

		if (possibility.exists("pub-key") && this.m_PubKey != null && possibility.extractString("pub-key").equals(this.m_PubKey) == false)
			return false;
		
		if (possibility.exists("keywords") && checkList(possibility.extractItem("keywords"), "keyword", this.m_Keywords) == false)
			return false;

		return true;
	}

	private boolean checkList(SNode node, String extractName, List<String> constraintList) {
		if (constraintList.size() == 0)
			return false;
		
		String candidate;

		for (int i = 0; i < node.count(); i++) {
			candidate = node.extractString(extractName);
			for (String constraint : constraintList) {
				if (candidate.equals(constraint)) {
					return true;
				}
			}
		}
		return false;
	}
}
