



































package org.mozilla.xpcom;

import java.io.File;

public interface IMozilla {

	









	void initialize(File aLibXULDirectory) throws XPCOMInitializationException;

	






	long getNativeHandleFromAWT(Object widget);
}
