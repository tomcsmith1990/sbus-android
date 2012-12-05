package uk.ac.cam.tcs40.sbus;

public class SComponent {

	public SComponent(String componentType, String componentName) {
		scomponent(componentType, componentName);
	}
	
	public native void scomponent(String componentType, String componentName);
	public native void addEndpoint(String endpointName, String endpointHash);
	public native void addRDC(String rdcAddress);
	public native void start(String cptFilename, int port, boolean useRDC);
	public native void setPermission(String cptName, String something, boolean allow);
	
	public native String emit(String message, int val1, int val2);
	public native String emit(SNode msg, SNode someval, SNode somevar);
	public native String emit(SNode[] nodes);
	
	public native void pack(SNode node);
	public native String emit();
	
	public native void delete();
	
	static {
        System.loadLibrary("somesensor");
    }	
}
