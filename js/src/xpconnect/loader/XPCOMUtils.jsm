














































EXPORTED_SYMBOLS = [ "XPCOMUtils" ];

debug("*** loading XPCOMUtils\n");

var XPCOMUtils = {

  


















  generateFactory: function(ctor, interfaces) {
    return {
      createInstance: function(outer, iid) {
        if (outer) throw Components.results.NS_ERROR_NO_AGGREGATION;
        if (!interfaces)
          return (new ctor()).QueryInterface(iid);
        for (var i=interfaces.length; i>=0; --i) {
          if (iid.equals(interfaces[i])) break;
        }
        if (i < 0 && !iid.equals(Components.interfaces.nsISupports))
          throw Components.results.NS_ERROR_NO_INTERFACE;
        return (new ctor());
      }
    }
  },
  
  


















  generateNSGetModule: function(classArray, postRegister, preUnregister) {
    var module = {
      getClassObject: function(compMgr, cid, iid) {
        if (!iid.equals(Components.interfaces.nsIFactory))
          throw Components.results.NS_ERROR_NO_INTERFACE;
        for (var i=0; i<classArray.length; ++i) {
          if (cid.equals(classArray[i].cid))
            return classArray[i].factory;
        }
        throw Components.results.NS_ERROR_NOT_IMPLEMENTED;
      },
      registerSelf: function(compMgr, fileSpec, location, type) {
        debug("*** registering " + fileSpec.leafName + ": [ ");
        compMgr =
          compMgr.QueryInterface(Components.interfaces.nsIComponentRegistrar);
        for (var i=0; i < classArray.length; ++i) {
          debug(classArray[i].className + " ");
          compMgr.registerFactoryLocation(classArray[i].cid,
                                          classArray[i].className,
                                          classArray[i].contractID,
                                          fileSpec,
                                          location,
                                          type);
        }
        if (postRegister)
          postRegister(compMgr, fileSpec, classArray);
        debug("]\n");
      },
      unregisterSelf: function(compMgr, fileSpec, location) {
        debug("*** unregistering " + fileSpec.leafName + ": [ ");
        if (preUnregister)
          preUnregister(compMgr, fileSpec, classArray);
        compMgr =
          compMgr.QueryInterface(Components.interfaces.nsIComponentRegistrar);
        for (var i=0; i < classArray.length; ++i) {
          debug(classArray[i].className + " ");
          compMgr.unregisterFactoryLocation(classArray[i].cid, fileSpec);
        }
        debug("]\n");
      },
      canUnload: function(compMgr) {
        return true;
      }
    };

    return function(compMgr, fileSpec) {
      return module;
    }
  },

  


  get categoryManager() {
    return Components.classes["@mozilla.org/categorymanager;1"]
           .getService(Components.interfaces.nsICategoryManager);
  }
};
