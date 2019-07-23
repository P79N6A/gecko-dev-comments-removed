



































package org.mozilla.xpcom.internal;

import java.io.File;

import org.mozilla.xpcom.IAppFileLocProvider;
import org.mozilla.xpcom.IXPCOM;

import org.mozilla.interfaces.nsIComponentManager;
import org.mozilla.interfaces.nsIComponentRegistrar;
import org.mozilla.interfaces.nsILocalFile;
import org.mozilla.interfaces.nsIServiceManager;


public class XPCOMImpl implements IXPCOM {

  public nsIServiceManager initXPCOM(File aMozBinDirectory,
          IAppFileLocProvider aAppFileLocProvider) {
    return initXPCOMNative(aMozBinDirectory, aAppFileLocProvider);
  }

  public native nsIServiceManager initXPCOMNative(File aMozBinDirectory,
          IAppFileLocProvider aAppFileLocProvider);

  public native void shutdownXPCOM(nsIServiceManager aServMgr);

  public native nsIComponentManager getComponentManager();

  public native nsIComponentRegistrar getComponentRegistrar();

  public native nsIServiceManager getServiceManager();

  public native nsILocalFile newLocalFile(String aPath, boolean aFollowLinks);

}

