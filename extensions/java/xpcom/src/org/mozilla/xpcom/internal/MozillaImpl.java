



































package org.mozilla.xpcom.internal;

import java.io.File;

import org.mozilla.xpcom.IMozilla;
import org.mozilla.xpcom.XPCOMInitializationException;

public class MozillaImpl implements IMozilla {

	public void initialize(File aLibXULDirectory)
	throws XPCOMInitializationException {
		JavaXPCOMMethods.registerJavaXPCOMMethods(aLibXULDirectory);
		initializeNative();
	}

	private native void initializeNative();

	public native long getNativeHandleFromAWT(Object widget);

}
