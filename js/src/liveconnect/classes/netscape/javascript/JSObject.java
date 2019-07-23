













































package netscape.javascript;

import java.applet.Applet;
































public final class JSObject {
    
    private int                               internal;
    private long                              long_internal;

    


    private static native void initClass();
    static {
    	
    	String liveConnectLibrary = System.getProperty("netscape.jsj.dll", null);
    	if (liveConnectLibrary != null) {
			System.loadLibrary(liveConnectLibrary);
			initClass();
		}
    }

    


    private JSObject(int jsobj_addr) {
        internal = jsobj_addr;
    }

    private JSObject(long jsobj_addr) {
        long_internal = jsobj_addr;
    }

    



    public native Object	getMember(String name);

    




    public native Object	getSlot(int index);

    



    public native void 		setMember(String name, Object value);

    






    public native void 		setSlot(int index, Object value);

    


    public native void 		removeMember(String name);

    



    public native Object	call(String methodName, Object args[]);

    




    public native Object	eval(String s);

    


    public native String        toString();

    
    
  


    


    public static native JSObject	getWindow(Applet applet);

    



    protected native void	finalize();

    



    public native boolean equals(Object obj);
}
