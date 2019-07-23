








































function nsExtensionManager()
{
}

nsExtensionManager.prototype = {
  
  
  installExtension: function (aXPIFile, aFlags)
  {
    throw Components.results.NS_ERROR_NOT_IMPLEMENTED;
  },
  
  installTheme: function (aJARFile, aFlags)
  {
    throw Components.results.NS_ERROR_NOT_IMPLEMENTED;
  },
  
  
  
  getInterfaces: function (aCount)
  {
    var interfaces = [Components.interfaces.nsIExtensionManager];
    aCount.value = interfaces.length;
    return interfaces;
  },
  
  getHelperForLanguage: function (aLanguage)
  {
    return null;
  },
  
  get contractID() 
  {
    return "@mozilla.org/extensions/manager;1";
  },
  
  get classDescription()
  {
    return "Extension Manager";
  },
  
  get classID() 
  {
    return Components.ID("{8A115FAA-7DCB-4e8f-979B-5F53472F51CF}");
  },
  
  get implementationLanguage()
  {
    return Components.interfaces.nsIProgrammingLanguage.JAVASCRIPT;
  },
  
  get flags()
  {
    return Components.interfaces.nsIClassInfo.SINGLETON;
  },

  
  
  QueryInterface: function (aIID) 
  {
    if (!aIID.equals(Components.interfaces.nsIExtensionManager))
      throw Components.results.NS_ERROR_NO_INTERFACE;
    return this;
  }
};

var gModule = {
  _firstTime: true,
  
  registerSelf: function (aComponentManager, aFileSpec, aLocation, aType) 
  {
    if (this._firstTime) {
      this._firstTime = false;
      throw Components.results.NS_ERROR_FACTORY_REGISTER_AGAIN;
    }
    aComponentManager = aComponentManager.QueryInterface(Components.interfaces.nsIComponentRegistrar);
    
    for (var key in this._objects) {
      var obj = this._objects[key];
      aComponentManager.registerFactoryLocation(obj.CID, obj.className, obj.contractID,
                                                aFileSpec, aLocation, aType);
    }
  },
  
  getClassObject: function (aComponentManager, aCID, aIID) 
  {
    if (!aIID.equals(Components.interfaces.nsIFactory))
      throw Components.results.NS_ERROR_NOT_IMPLEMENTED;

    for (var key in this._objects) {
      if (aCID.equals(this._objects[key].CID))
        return this._objects[key].factory;
    }
    
    throw Components.results.NS_ERROR_NO_INTERFACE;
  },
  
  _objects: {
    manager: { CID        : nsExtensionManager.prototype.classID,
               contractID : nsExtensionManager.prototype.contractID,
               className  : nsExtensionManager.prototype.classDescription,
               factory    : {
                              createInstance: function (aOuter, aIID) 
                              {
                                if (aOuter != null)
                                  throw Components.results.NS_ERROR_NO_AGGREGATION;
                                
                                return (new nsExtensionManager()).QueryInterface(aIID);
                              }
                            }
             }
   },
  
  canUnload: function (aComponentManager) 
  {
    return true;
  }
};

function NSGetModule(compMgr, fileSpec) 
{
  return gModule;
}
