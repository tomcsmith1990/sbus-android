package uk.ac.cam.tcs40.sbus;

public class SNode {

	private enum NodeType { SInt, SString };
	
	private String m_NodeName;
	private NodeType m_NodeType;
	
	private int n; // SInt, SBool, SValue (cooked)
	private double x; // SDouble
	private int len; // SBinary
	private String s; // SText, SValue (raw)

	//private char *data; // SBinary
	
	public SNode(int n) {
		this(n, null);
	}
	
	public SNode(int n, String name) {
		initialise();
		this.n = n;
		this.m_NodeName = name;
		this.m_NodeType = NodeType.SInt;
	}
	
	public SNode(String s) {
		this(s, null);
	}
	
	public SNode(String s, String name) {
		initialise();
		this.s = s;
		this.m_NodeName = name;
		this.m_NodeType = NodeType.SString;
	}
	
	private void initialise() {
		this.s = null;
		this.n = -1;
		this.m_NodeName = null;
	}
	
	private int getNodeType() {
		return this.m_NodeType.ordinal();
	}
	
	private String getNodeName() {
		return this.m_NodeName;
	}
	
	private int getIntValue() {
		return this.n;
	}
	
	private String getStringValue() {
		return this.s;
	}
}
