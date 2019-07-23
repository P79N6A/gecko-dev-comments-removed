








































const REFTEST_CMDLINE_CONTRACTID     = "@mozilla.org/commandlinehandler/general-startup;1?type=reftest";
const REFTEST_CMDLINE_CLSID          = Components.ID('{32530271-8c1b-4b7d-a812-218e42c6bb23}');
const CATMAN_CONTRACTID              = "@mozilla.org/categorymanager;1";
const nsISupports                    = Components.interfaces.nsISupports;
  
const nsICategoryManager             = Components.interfaces.nsICategoryManager;
const nsICmdLineHandler              = Components.interfaces.nsICmdLineHandler;
const nsICommandLine                 = Components.interfaces.nsICommandLine;
const nsICommandLineHandler          = Components.interfaces.nsICommandLineHandler;
const nsIComponentRegistrar          = Components.interfaces.nsIComponentRegistrar;
const nsISupportsString              = Components.interfaces.nsISupportsString;
const nsIWindowWatcher               = Components.interfaces.nsIWindowWatcher;

function RefTestCmdLineHandler() {}
RefTestCmdLineHandler.prototype =
{
  
  QueryInterface : function handler_QI(iid) {
    if (iid.equals(nsISupports))
      return this;

    if (nsICmdLineHandler && iid.equals(nsICmdLineHandler))
      return this;

    if (nsICommandLineHandler && iid.equals(nsICommandLineHandler))
      return this;

    throw Components.results.NS_ERROR_NO_INTERFACE;
  },

  
  commandLineArgument : "-reftest",
  prefNameForStartup : "general.startup.reftest",
  chromeUrlForTask : "chrome://reftest/content/reftest.xul",
  helpText : "Run layout acceptance tests on given manifest.",
  handlesArgs : true,
  defaultArgs : "",
  openWindowWithArgs : true,

  
  handle : function handler_handle(cmdLine) {
    var args = { };
    args.wrappedJSObject = args;
    try {
      var uristr = cmdLine.handleFlagWithParam("reftest", false);
      if (uristr == null)
        return;
      try {
        args.uri = cmdLine.resolveURI(uristr).spec;
      }
      catch (e) {
        return;
      }
    }
    catch (e) {
      cmdLine.handleFlag("reftest", true);
    }

    try {
      var nocache = cmdLine.handleFlag("reftestnocache", false);
      args.nocache = nocache;
    }
    catch (e) {
    }

    
    var ios = Components.classes["@mozilla.org/network/io-service;1"]
              .getService(Components.interfaces.nsIIOService2);
    ios.manageOfflineStatus = false;
    ios.offline = false;

    

    var prefs = Components.classes["@mozilla.org/preferences-service;1"].
                getService(Components.interfaces.nsIPrefBranch2);
    prefs.setBoolPref("gfx.color_management.force_srgb", true);

    var wwatch = Components.classes["@mozilla.org/embedcomp/window-watcher;1"]
                           .getService(nsIWindowWatcher);
    wwatch.openWindow(null, "chrome://reftest/content/reftest.xul", "_blank",
                      "chrome,dialog=no,all", args);
    cmdLine.preventDefault = true;
  },

  helpInfo : "  -reftest <file>    Run layout acceptance tests on given manifest.\n"
};


var RefTestCmdLineFactory =
{
  createInstance : function(outer, iid)
  {
    if (outer != null) {
      throw Components.results.NS_ERROR_NO_AGGREGATION;
    }

    return new RefTestCmdLineHandler().QueryInterface(iid);
  }
};


var RefTestCmdLineModule =
{
  registerSelf : function(compMgr, fileSpec, location, type)
  {
    compMgr = compMgr.QueryInterface(nsIComponentRegistrar);

    compMgr.registerFactoryLocation(REFTEST_CMDLINE_CLSID,
                                    "RefTest CommandLine Service",
                                    REFTEST_CMDLINE_CONTRACTID,
                                    fileSpec,
                                    location,
                                    type);

    var catman = Components.classes[CATMAN_CONTRACTID].getService(nsICategoryManager);
    catman.addCategoryEntry("command-line-argument-handlers",
                            "reftest command line handler",
                            REFTEST_CMDLINE_CONTRACTID, true, true);
    catman.addCategoryEntry("command-line-handler",
                            "m-reftest",
                            REFTEST_CMDLINE_CONTRACTID, true, true);
  },

  unregisterSelf : function(compMgr, fileSpec, location)
  {
    compMgr = compMgr.QueryInterface(nsIComponentRegistrar);

    compMgr.unregisterFactoryLocation(REFTEST_CMDLINE_CLSID, fileSpec);
    catman = Components.classes[CATMAN_CONTRACTID].getService(nsICategoryManager);
    catman.deleteCategoryEntry("command-line-argument-handlers",
                               "reftest command line handler", true);
    catman.deleteCategoryEntry("command-line-handler",
                               "m-reftest", true);
  },

  getClassObject : function(compMgr, cid, iid)
  {
    if (cid.equals(REFTEST_CMDLINE_CLSID)) {
      return RefTestCmdLineFactory;
    }

    if (!iid.equals(Components.interfaces.nsIFactory)) {
      throw Components.results.NS_ERROR_NOT_IMPLEMENTED;
    }

    throw Components.results.NS_ERROR_NO_INTERFACE;
  },

  canUnload : function(compMgr)
  {
    return true;
  }
};


function NSGetModule(compMgr, fileSpec) {
  return RefTestCmdLineModule;
}
