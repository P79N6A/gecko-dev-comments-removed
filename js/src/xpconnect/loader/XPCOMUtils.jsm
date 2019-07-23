







































































































var EXPORTED_SYMBOLS = [ "XPCOMUtils" ];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

var XPCOMUtils = {
  





  generateQI: function(interfaces) {
    
    return makeQI([Ci[i].name for each (i in interfaces) if (Ci[i])]);
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
        categories:   component.prototype._xpcom_categories
      });
    }

    return { 
      getClassObject: function(compMgr, cid, iid) {
        
        if (!iid.equals(Ci.nsIFactory))
          throw Cr.NS_ERROR_NOT_IMPLEMENTED;

        for each (let classDesc in classes) {
          if (classDesc.cid.equals(cid))
            return classDesc.factory;
        }

        throw Cr.NS_ERROR_FACTORY_NOT_REGISTERED;
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
          if (classDesc.categories) {
            let catMan = XPCOMUtils.categoryManager;
            for each (let cat in classDesc.categories) {
              let defaultValue = (cat.service ? "service," : "") +
                                 classDesc.contractID;
              catMan.addCategoryEntry(cat.category,
                                      cat.entry || classDesc.className,
                                      cat.value || defaultValue,
                                      true, true);
            }
          }
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
          if (classDesc.categories) {
            let catMan = XPCOMUtils.categoryManager;
            for each (let cat in classDesc.categories) {
              catMan.deleteCategoryEntry(cat.category,
                                         cat.entry || classDesc.className,
                                         true);
            }
          }
          compMgr.unregisterFactoryLocation(classDesc.cid, fileSpec);
        }
        debug(" ]\n");
      },

      canUnload: function(compMgr) {
        return true;
      }
    };
  },

  










  defineLazyGetter: function XPCU_defineLazyGetter(aObject, aName, aLambda)
  {
    aObject.__defineGetter__(aName, function() {
      delete aObject[aName];
      return aObject[aName] = aLambda();
    });
  },

  












  defineLazyServiceGetter: function XPCU_defineLazyServiceGetter(aObject, aName,
                                                                 aContract,
                                                                 aInterfaceName)
  {
    this.defineLazyGetter(aObject, aName, function XPCU_serviceLambda() {
      return Cc[aContract].getService(Ci[aInterfaceName]);
    });
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
          if (outer)
            throw Cr.NS_ERROR_NO_AGGREGATION;
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

    throw Cr.NS_ERROR_NO_INTERFACE;
  };
}
