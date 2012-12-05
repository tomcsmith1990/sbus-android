package uk.ac.cam.tcs40.sbus;

public class SNode {

	private enum NodeType { SInt };
	
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
	
	private int getInt() {
		return this.n;
	}
}
