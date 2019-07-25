












































































































var EXPORTED_SYMBOLS = [ "XPCOMUtils" ];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

var XPCOMUtils = {
  





  generateQI: function XPCU_generateQI(interfaces) {
    
    return makeQI([Ci[i].name for each (i in interfaces) if (Ci[i])]);
  },

  



  generateNSGetModule: function XPCU_generateNSGetModule(componentsArray,
                                                         postRegister,
                                                         preUnregister) {
    return function NSGetModule(compMgr, fileSpec) {
      return XPCOMUtils.generateModule(componentsArray,
                                       postRegister,
                                       preUnregister);
    }
  },

  











  generateModule: function XPCU_generateModule(componentsArray,
                                               postRegister,
                                               preUnregister) {
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

    function categoryRegistration(action, compMgr, fileSpec,
                                  registrationFunc, hookFunc) {
      debug("*** " + action + "ing " + fileSpec.leafName + ": [ ");
      var componentCount = 0;
      compMgr.QueryInterface(Ci.nsIComponentRegistrar);

      if (action == "unregister" && preUnregister)
        preUnregister(compMgr, fileSpec, componentsArray);

      for each (let classDesc in classes) {
        debug((componentCount++ ? ", " : "") + classDesc.className);

        if (action == "register" && hookFunc)
          hookFunc(classDesc);

        if (classDesc.categories) {
          for each (let cat in classDesc.categories) {
            if ("apps" in cat && -1 == cat.apps.indexOf(XPCOMUtils._appID))
              continue;
            registrationFunc(cat, classDesc);
          }
        }

        if (action == "unregister" && hookFunc)
          hookFunc(classDesc);
      }

      if (action == "register" && postRegister)
        postRegister(compMgr, fileSpec, componentsArray);
      debug(" ]\n");
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
        categoryRegistration("register", compMgr, fileSpec,
          function(cat, classDesc) {
            let defaultValue = (cat.service ? "service," : "") +
                               classDesc.contractID;
            let catMan = XPCOMUtils.categoryManager;
            catMan.addCategoryEntry(cat.category,
                                    cat.entry || classDesc.className,
                                    cat.value || defaultValue,
                                    true, true);
          },
          function(classDesc) {
            compMgr.registerFactoryLocation(classDesc.cid,
                                            classDesc.className,
                                            classDesc.contractID,
                                            fileSpec,
                                            location,
                                            type);
          });
      },

      unregisterSelf: function(compMgr, fileSpec, location) {
        categoryRegistration("unregister", compMgr, fileSpec,
          function(cat, classDesc) {
            let catMan = XPCOMUtils.categoryManager;
            catMan.deleteCategoryEntry(cat.category,
                                       cat.entry || classDesc.className,
                                       true);
          },
          function (classDesc) {
            compMgr.unregisterFactoryLocation(classDesc.cid, fileSpec);
          });
      },

      canUnload: function(compMgr) {
        return true;
      }
    };
  },

  get _appID() {
    delete this._appID;
    let appInfo = Cc["@mozilla.org/xre/app-info;1"].
                  getService(Ci.nsIXULAppInfo);
    return this._appID = appInfo.ID;
  },

  










  defineLazyGetter: function XPCU_defineLazyGetter(aObject, aName, aLambda)
  {
    aObject.__defineGetter__(aName, function() {
      delete aObject[aName];
      return aObject[aName] = aLambda.apply(aObject);
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

  




  IterSimpleEnumerator: function XPCU_IterSimpleEnumerator(e, i)
  {
    while (e.hasMoreElements())
      yield e.getNext().QueryInterface(i);
  },

  




  IterStringEnumerator: function XPCU_IterStringEnumerator(e)
  {
    while (e.hasMore())
      yield e.getNext();
  },

  


  _getFactory: function XPCOMUtils__getFactory(component) {
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

