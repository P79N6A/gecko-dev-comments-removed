




































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

const CID = Components.ID("{b69155f4-a8bf-453d-8653-91d1456e1d3d}");
const CONTRACT_ID = "@mozilla.org/appshell/trytoclose;1"
const CLASS_NAME = "tryToClose Service";

function TryToClose() {
}

TryToClose.prototype = {
  observe: function (aSubject, aTopic, aData) {
    switch (aTopic) {
    case "app-startup":
      var obsService = Cc["@mozilla.org/observer-service;1"].
                       getService(Ci.nsIObserverService);
      obsService.addObserver(this, "quit-application-requested", true);
      break;
    case "quit-application-requested":
      var windowMediator = Cc['@mozilla.org/appshell/window-mediator;1'].
                           getService(Ci.nsIWindowMediator);
      var enumerator = windowMediator.getEnumerator(null);
      while (enumerator.hasMoreElements()) {
        var domWindow = enumerator.getNext();
        if (("tryToClose" in domWindow) && !domWindow.tryToClose()) {
          aSubject.QueryInterface(Ci.nsISupportsPRBool);
          aSubject.data = true;
          break;
        }
      }
      break;
    }
  },

  QueryInterface: function(aIID) {
    if (!aIID.equals(Ci.nsIObserver) &&
        !aIID.equals(Ci.nsISupportsWeakReference) && 
        !aIID.equals(Ci.nsISupports))
      throw Cr.NS_ERROR_NO_INTERFACE;
    return this;
  }
};

const TryToCloseFactory = {
  createInstance: function(aOuter, aIID) {
    if (aOuter != null)
      throw Cr.NS_ERROR_NO_AGGREGATION;
    
    return (new TryToClose()).QueryInterface(aIID);
  }
};

const TryToCloseModule = {
  registerSelf: function(aCompMgr, aFileSpec, aLocation, aType) {
    aCompMgr.QueryInterface(Ci.nsIComponentRegistrar);
    aCompMgr.registerFactoryLocation(CID, CLASS_NAME, CONTRACT_ID,
                                     aFileSpec, aLocation, aType);

    var catMan = Cc["@mozilla.org/categorymanager;1"].
                 getService(Ci.nsICategoryManager);
    catMan.addCategoryEntry("app-startup", CLASS_NAME, "service," + CONTRACT_ID, true, true);
  },

  unregisterSelf: function(aCompMgr, aLocation, aType) {
    aCompMgr.QueryInterface(Ci.nsIComponentRegistrar);
    aCompMgr.unregisterFactoryLocation(CID, aLocation);

    var catMan = Cc["@mozilla.org/categorymanager;1"].
                 getService(Ci.nsICategoryManager);
    catMan.deleteCategoryEntry( "app-startup", "service," + CONTRACT_ID, true);
  },

  getClassObject: function(aCompMgr, aCID, aIID) {
    if (aCID.equals(CID))
      return TryToCloseFactory;
    
    throw Cr.NS_ERROR_NOT_REGISTERED;
  },

  canUnload: function(aCompMgr) {
    return true;
  }
};

function NSGetModule(aCompMgr, aFileSpec) {
  return TryToCloseModule;
}
