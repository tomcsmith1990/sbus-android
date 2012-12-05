package uk.ac.cam.tcs40.sbus;

public class SNode {

	private enum NodeType { SInt, SString };
	
	private String name;
	private NodeType type;
	
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
		this.name = name;
		this.type = NodeType.SInt;
	}
	
	public SNode(String s) {
		this(s, null);
	}
	
	public SNode(String s, String name) {
		initialise();
		this.s = s;
		this.name = name;
		this.type = NodeType.SString;
	}
	
	private void initialise() {
		this.s = null;
		this.n = -1;
		this.name = null;
	}
	
	private int getNodeType() {
		return this.type.ordinal();
	}
	
	private String getNodeName() {
		return this.name;
	}
	
	private int getIntValue() {
		return this.n;
	}
	
	private String getStringValue() {
		return this.s;
	}
}
