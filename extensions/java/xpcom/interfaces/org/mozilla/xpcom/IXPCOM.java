



































package org.mozilla.xpcom;

import java.io.File;

import org.mozilla.interfaces.nsIComponentManager;
import org.mozilla.interfaces.nsIComponentRegistrar;
import org.mozilla.interfaces.nsILocalFile;
import org.mozilla.interfaces.nsIServiceManager;


public interface IXPCOM {

  






















  nsIServiceManager initXPCOM(File aMozBinDirectory,
          IAppFileLocProvider aAppFileLocProvider) throws XPCOMException;

  








  void shutdownXPCOM(nsIServiceManager aServMgr) throws XPCOMException;

  






  nsIServiceManager getServiceManager() throws XPCOMException;

  






  nsIComponentManager getComponentManager() throws XPCOMException;

  






  nsIComponentRegistrar getComponentRegistrar() throws XPCOMException;

  

















  nsILocalFile newLocalFile(String aPath, boolean aFollowLinks)
          throws XPCOMException;

}

