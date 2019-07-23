











































function nsSetDefaultBrowser() {
}

nsSetDefaultBrowser.prototype = {
  
  QueryInterface: function nsSetDefault_QI(iid) {
    if (!iid.equals(Components.interfaces.nsICommandLineHandler) &&
        !iid.equals(Components.interfaces.nsISupports))
      throw Components.results.NS_ERROR_NO_INTERFACE;

    return this;
  },

  
  handle : function nsSetDefault_handle(cmdline) {
    if (cmdline.handleFlag("setDefaultBrowser", false)) {
      var shell = Components.classes["@mozilla.org/browser/shell-service;1"]
                            .getService(Components.interfaces.nsIShellService);
      shell.setDefaultBrowser(true, true);
    }
  },

  helpInfo : "  -setDefaultBrowser   Set this app as the default browser.\n"
}



const contractID = "@mozilla.org/browser/default-browser-clh;1";
const CID = Components.ID("{F57899D0-4E2C-4ac6-9E29-50C736103B0C}");

var ModuleAndFactory = {
  
  QueryInterface: function nsSetDefault_QI(iid) {
    if (!iid.equals(Components.interfaces.nsIModule) &&
        !iid.equals(Components.interfaces.nsIFactory) &&
        !iid.equals(Components.interfaces.nsISupports))
      throw Components.results.NS_ERROR_NO_INTERFACE;

    return this;
  },

  
  getClassObject: function (compMgr, cid, iid) {
    if (!cid.equals(CID))
      throw Components.results.NS_ERROR_NO_INTERFACE;
    
    return this.QueryInterface(iid);
  },
    
  registerSelf: function mod_regself(compMgr, fileSpec, location, type) {
    var compReg =
      compMgr.QueryInterface( Components.interfaces.nsIComponentRegistrar );

    compReg.registerFactoryLocation( CID,
                                     "Default Browser Cmdline Handler",
                                     contractID,
                                     fileSpec,
                                     location,
                                     type );

    var catMan = Components.classes["@mozilla.org/categorymanager;1"]
                           .getService(Components.interfaces.nsICategoryManager);

    catMan.addCategoryEntry("command-line-handler",
                            "m-setdefaultbrowser",
                            contractID, true, true);
  },
    
  unregisterSelf : function mod_unregself(compMgr, location, type) {
    var catMan = Components.classes["@mozilla.org/categorymanager;1"]
                           .getService(Components.interfaces.nsICategoryManager);

    catMan.deleteCategoryEntry("command-line-handler",
                               "m-setdefaultbrowser", true);
  },

  canUnload: function(compMgr) {
    return true;
  },

  
  createInstance: function mod_CI(outer, iid) {
    if (outer != null)
      throw Components.results.NS_ERROR_NO_AGGREGATION;
  
    return new nsSetDefaultBrowser().QueryInterface(iid);
  },
    
  lockFactory : function mod_lock(lock) {
    
  }
}


function NSGetModule(compMgr, fileSpec) {
  return ModuleAndFactory;
}
