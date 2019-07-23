



















































































EXPORTED_SYMBOLS = [ "XPCOMUtils" ];

const Ci = Components.interfaces;

var XPCOMUtils = {
  





  generateQI: function(interfaces) {
    return makeQI([i.name for each(i in interfaces)]);
  },

  



  generateNSGetModule: function(componentsArray, postRegister, preUnregister) {
    return function NSGetModule(compMgr, fileSpec) {
      return XPCOMUtils.generateModule(componentsArray,
                                       postRegister,
                                       preUnregister);
    }
  },

  











  generateModule: function(componentsArray, postRegister, preUnregister) {
    let classes = [];
    for each (let component in componentsArray) {
      classes.push({
        cid:          component.prototype.classID,
        className:    component.prototype.classDescription,
        contractID:   component.prototype.contractID,
        factory:      this._getFactory(component),
      });
    }

    return { 
      getClassObject: function(compMgr, cid, iid) {
        if (!iid.equals(Ci.nsIFactory))
          throw Components.results.NS_ERROR_NO_INTERFACE;

        for each (let classDesc in classes) {
          if (classDesc.cid.equals(cid))
            return classDesc.factory;
        }

        throw Components.results.NS_ERROR_NOT_IMPLEMENTED;
      },

      registerSelf: function(compMgr, fileSpec, location, type) {
        var componentCount = 0;
        debug("*** registering " + fileSpec.leafName + ": [ ");
        compMgr.QueryInterface(Ci.nsIComponentRegistrar);
        for each (let classDesc in classes) {
          debug((componentCount++ ? ", " : "") + classDesc.className);
          compMgr.registerFactoryLocation(classDesc.cid,
                                          classDesc.className,
                                          classDesc.contractID,
                                          fileSpec,
                                          location,
                                          type);
        }

        if (postRegister)
          postRegister(compMgr, fileSpec, componentsArray);
        debug(" ]\n");
      },

      unregisterSelf: function(compMgr, fileSpec, location) {
        var componentCount = 0;
        debug("*** unregistering " + fileSpec.leafName + ": [ ");
        compMgr.QueryInterface(Ci.nsIComponentRegistrar);
        if (preUnregister)
          preUnregister(compMgr, fileSpec, componentsArray);

        for each (let classDesc in classes) {
          debug((componentCount++ ? ", " : "") + classDesc.className);
          compMgr.unregisterFactoryLocation(classDesc.cid, fileSpec);
        }
        debug(" ]\n");
      },

      canUnload: function(compMgr) {
        return true;
      }
    };
  },

  


  get categoryManager() {
    return Components.classes["@mozilla.org/categorymanager;1"]
           .getService(Ci.nsICategoryManager);
  },

  


  _getFactory: function(component) {
    var factory = component.prototype._xpcom_factory;
    if (!factory) {
      factory = {
        createInstance: function(outer, iid) {
          if(outer)
            throw CR.NS_ERROR_NO_AGGREGATION;
          return (new component()).QueryInterface(iid);
        }
      }
    }
    return factory;
  }
};




function makeQI(interfaceNames) {
  return function XPCOMUtils_QueryInterface(iid) {
    if (iid.equals(Ci.nsISupports))
      return this;
    for each(let interfaceName in interfaceNames) {
      if (Ci[interfaceName].equals(iid))
        return this;
    }

    throw Components.results.NS_ERROR_NO_INTERFACE;
  };
}
