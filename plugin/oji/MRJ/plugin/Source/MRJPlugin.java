







































import netscape.plugin.Plugin;

class MRJPlugin extends Plugin {
	MRJPlugin() {}

	public native Object getField(String name, String signature);	
	public native void setField(String name, String signature, Object value);
	public native Object callMethod(String name, String signature, Object[] args);
}
