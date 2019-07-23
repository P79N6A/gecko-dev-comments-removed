



































const nsICmdLineHandler     = Components.interfaces.nsICmdLineHandler;
const nsICommandLineHandler = Components.interfaces.nsICommandLineHandler;
const nsIFactory            = Components.interfaces.nsIFactory;
const nsISupports           = Components.interfaces.nsISupports;
const nsIModule             = Components.interfaces.nsIModule;
const nsIComponentRegistrar = Components.interfaces.nsIComponentRegistrar;
const nsICategoryManager    = Components.interfaces.nsICategoryManager;
const nsISupportsString     = Components.interfaces.nsISupportsString;
const nsIWindowWatcher      = Components.interfaces.nsIWindowWatcher;

const NS_ERROR_FAILURE        = Components.results.NS_ERROR_FAILURE;
const NS_ERROR_NO_AGGREGATION = Components.results.NS_ERROR_NO_AGGREGATION;
const NS_ERROR_NO_INTERFACE   = Components.results.NS_ERROR_NO_INTERFACE;

function nsComposerCmdLineHandler() {}
nsComposerCmdLineHandler.prototype = {
  get wrappedJSObject() {
    return this;
  },

  

  QueryInterface: function(iid) {
    if (iid.equals(nsISupports))
      return this;

    if (nsICmdLineHandler && iid.equals(nsICmdLineHandler))
      return this;

    if (nsICommandLineHandler && iid.equals(nsICommandLineHandler))
      return this;

    throw NS_ERROR_NO_INTERFACE;
  },

  
  commandLineArgument : "-edit",
  prefNameForStartup : "general.startup.editor",
  chromeUrlForTask : "chrome://editor/content/editor.xul",
  helpText : "Start with editor.",
  handlesArgs : true,
  defaultArgs : "about:blank",
  openWindowWithArgs : true,

  
  handle : function handle(cmdLine) {
    var args = Components.classes["@mozilla.org/supports-string;1"]
                         .createInstance(nsISupportsString);
    try {
      var uristr = cmdLine.handleFlagWithParam("edit", false);
      if (uristr == null) {
        
        uristr = cmdLine.handleFlagWithParam("editor", false);
        if (uristr == null)
          return;
      }

      try {
        args.data = cmdLine.resolveURI(uristr).spec;
      }
      catch (e) {
        return;
      }
    }
    catch (e) {
      
      args.data = "about:blank";
    }

    var wwatch = Components.classes["@mozilla.org/embedcomp/window-watcher;1"]
                           .getService(nsIWindowWatcher);
    wwatch.openWindow(null, "chrome://editor/content", "_blank",
                      "chrome,dialog=no,all", args);
    cmdLine.preventDefault = true;
  },

  helpInfo : "  -edit <url>          Open Composer.\n"
};

function nsComposerCmdLineHandlerFactory() {
}

nsComposerCmdLineHandlerFactory.prototype = {
  

  QueryInterface: function(iid) {
    if (!iid.equals(nsIFactory) &&
        !iid.equals(nsISupports)) {
          throw Components.results.NS_ERROR_NO_INTERFACE;
    }
    return this;
  },

  
  createInstance: function(outer, iid) {
    if (outer != null) {
      throw NS_ERROR_NO_AGGREGATION;
    }

    return new nsComposerCmdLineHandler().QueryInterface(iid);
  },

  lockFactory: function(lock) {
  }
};

const nsComposerCmdLineHandler_CID =
  Components.ID("{f7d8db95-ab5d-4393-a796-9112fe758cfa}");

const ContractIDPrefix =
  "@mozilla.org/commandlinehandler/general-startup;1?type=";

var thisModule = {
  

  QueryInterface: function(iid) {
    if (!iid.equals(nsIModule) &&
        !iid.equals(nsISupports)) {
          throw Components.results.NS_ERROR_NO_INTERFACE;
    }
    return this;
  },

  

  getClassObject: function (compMgr, cid, iid) {
    if (!cid.equals(nsComposerCmdLineHandler_CID)) {
      throw NS_ERROR_FAILURE;
    }

    if (!iid.equals(nsIFactory)) {
      throw NS_ERROR_NO_INTERFACE;
    }

    return new nsComposerCmdLineHandlerFactory();
  },

  registerSelf: function (compMgr, fileSpec, location, type) {
    var compReg = compMgr.QueryInterface(nsIComponentRegistrar);
    compReg.registerFactoryLocation(nsComposerCmdLineHandler_CID,
                                    "nsComposerCmdLineHandler",
                                    ContractIDPrefix + "edit",
                                    fileSpec, location, type);
    compReg.registerFactoryLocation(nsComposerCmdLineHandler_CID,
                                    "nsComposerCmdLineHandler",
                                    ContractIDPrefix + "editor",
                                    fileSpec, location, type);

    var catMan = Components.classes["@mozilla.org/categorymanager;1"].getService(nsICategoryManager);
    catMan.addCategoryEntry("command-line-argument-handlers",
                            "nsComposerCmdLineHandler",
                            ContractIDPrefix + "edit",
                            true, true);
    catMan.addCategoryEntry("command-line-handler",
                            "m-edit",
                            ContractIDPrefix + "edit",
                            true, true);
  },

  unregisterSelf: function (compMgr, location, type) {
    var compReg = compMgr.QueryInterface(nsIComponentRegistrar);
    compReg.unregisterFactoryLocation(nsComposerCmdLineHandler_CID,
                                      location);

    var catMan = Components.classes["@mozilla.org/categorymanager;1"].getService(nsICategoryManager);
    catMan.deleteCategoryEntry("command-line-argument-handlers",
                               "nsComposerCmdLineHandler", true);
    catMan.deleteCategoryEntry("command-line-handler",
                               "m-edit", true);
  },    

  canUnload: function (compMgr) {
    return true;
  }
};

function NSGetModule(compMgr, fileSpec) {
  return thisModule;
}
