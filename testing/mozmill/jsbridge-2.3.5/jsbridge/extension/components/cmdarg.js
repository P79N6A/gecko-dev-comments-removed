const nsIAppShellService    = Components.interfaces.nsIAppShellService;
const nsISupports           = Components.interfaces.nsISupports;
const nsICategoryManager    = Components.interfaces.nsICategoryManager;
const nsIComponentRegistrar = Components.interfaces.nsIComponentRegistrar;
const nsICommandLine        = Components.interfaces.nsICommandLine;
const nsICommandLineHandler = Components.interfaces.nsICommandLineHandler;
const nsIFactory            = Components.interfaces.nsIFactory;
const nsIModule             = Components.interfaces.nsIModule;
const nsIWindowWatcher      = Components.interfaces.nsIWindowWatcher;


const CHROME_URI = "chrome://jsbridge/content/";



const clh_contractID = "@mozilla.org/commandlinehandler/general-startup;1?type=jsbridge";


const clh_CID = Components.ID("{2872d428-14f6-11de-ac86-001f5bd9235c}");



const clh_category = "jsbridge";

var aConsoleService = Components.classes["@mozilla.org/consoleservice;1"].
     getService(Components.interfaces.nsIConsoleService);










function openWindow(aChromeURISpec, aArgument)
{
  var ww = Components.classes["@mozilla.org/embedcomp/window-watcher;1"].
    getService(Components.interfaces.nsIWindowWatcher);
  ww.openWindow(null, aChromeURISpec, "_blank",
                "chrome,menubar,toolbar,status,resizable,dialog=no",
                aArgument);
}
 




const jsbridgeHandler = {
  
  QueryInterface : function clh_QI(iid)
  {
    if (iid.equals(nsICommandLineHandler) ||
        iid.equals(nsIFactory) ||
        iid.equals(nsISupports))
      return this;

    throw Components.results.NS_ERROR_NO_INTERFACE;
  },

  

  handle : function clh_handle(cmdLine)
  {
    try {
      var port = cmdLine.handleFlagWithParam("jsbridge", false);
      if (port) {
        var server = {}; 
        Components.utils.import('resource://jsbridge/modules/server.js', server);
        server.startServer(parseInt(port));
        
      } else {
        var server = {}; 
        Components.utils.import('resource://jsbridge/modules/server.js', server);
        server.startServer(24242);
      }
    }
    catch (e) {
      Components.utils.reportError("incorrect parameter passed to -jsbridge on the command line.");
    }

  },

  
  
  
  
  
  
  helpInfo : "  -jsbridge            Port to run jsbridge on.\n",

  

  createInstance : function clh_CI(outer, iid)
  {
    if (outer != null)
      throw Components.results.NS_ERROR_NO_AGGREGATION;

    return this.QueryInterface(iid);
  },

  lockFactory : function clh_lock(lock)
  {
    
  }
};




const jsbridgeHandlerModule = {
  
  QueryInterface : function mod_QI(iid)
  {
    if (iid.equals(nsIModule) ||
        iid.equals(nsISupports))
      return this;

    throw Components.results.NS_ERROR_NO_INTERFACE;
  },

  
  getClassObject : function mod_gch(compMgr, cid, iid)
  {
    if (cid.equals(clh_CID))
      return jsbridgeHandler.QueryInterface(iid);

    throw Components.results.NS_ERROR_NOT_REGISTERED;
  },

  registerSelf : function mod_regself(compMgr, fileSpec, location, type)
  {
    compMgr.QueryInterface(nsIComponentRegistrar);

    compMgr.registerFactoryLocation(clh_CID,
                                    "jsbridgeHandler",
                                    clh_contractID,
                                    fileSpec,
                                    location,
                                    type);

    var catMan = Components.classes["@mozilla.org/categorymanager;1"].
      getService(nsICategoryManager);
    catMan.addCategoryEntry("command-line-handler",
                            clh_category,
                            clh_contractID, true, true);
  },

  unregisterSelf : function mod_unreg(compMgr, location, type)
  {
    compMgr.QueryInterface(nsIComponentRegistrar);
    compMgr.unregisterFactoryLocation(clh_CID, location);

    var catMan = Components.classes["@mozilla.org/categorymanager;1"].
      getService(nsICategoryManager);
    catMan.deleteCategoryEntry("command-line-handler", clh_category);
  },

  canUnload : function (compMgr)
  {
    return true;
  }
};




function NSGetModule(comMgr, fileSpec)
{
  return jsbridgeHandlerModule;
}
