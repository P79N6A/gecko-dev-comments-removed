






































var gRegTestCoreComponent =
{
  
  QueryInterface: function (aIID)
  {
    if (!aIID.equals(Components.interfaces.nsISupports) &&
        !aIID.equals(Components.interfaces.nsIFactory))
      throw Components.results.NS_ERROR_NO_INTERFACE;

    return this;
  },

  
  createInstance: function (aOuter, aIID)
  {
    if (null != aOuter)
      throw Components.results.NS_ERROR_NO_AGGREGATION;

    return this.QueryInterface(aIID);
  },

  lockFactory: function (aDoLock) {}
};

var gRegTestCoreComponentModule =
{
  kClassName: "RegTestCoreComponent",
  kClassID: Components.ID("{56ab1cd4-ac44-4f86-8104-171f8b8f2fc7}"),
  kContractID: "@mozilla.org/RegTestServiceA;1",
  kICompReg: Components.interfaces.nsIComponentRegistrar,

  
  QueryInterface: function (aIID)
  {
    if (aIID.equals(Components.interfaces.nsIModule) ||
        aIID.equals(Components.interfaces.nsISupports))
      return this;

    throw Components.results.NS_ERROR_NO_INTERFACE;
  },

  
  getClassObject: function (aCompMgr, aClassID, aIID)
  {
    if (!aClassID.equals(this.kClassID))
      throw Components.results.NS_ERROR_NO_INTERFACE;
    if (!aIID.equals(Components.interfaces.nsIFactory))
      throw Components.results.NS_ERROR_NOT_IMPLEMENTED;

    return gRegTestCoreComponent.QueryInterface(aIID);
  },

  registerSelf: function (aCompMgr, aFileSpec, aLocation, aType)
  {
    aCompMgr = aCompMgr.QueryInterface(this.kICompReg);
    aCompMgr.registerFactoryLocation(this.kClassID, this.kClassName,
                                 this.kContractID, aFileSpec, aLocation, aType);
  },

  unregisterSelf: function (aCompMgr, aFileSpec, aLocation)
  {
    aCompMgr = aCompMgr.QueryInterface(this.kICompReg);
    aCompMgr.unregisterFactoryLocation(this.kClassID, aFileSpec);
  },

  canUnload: function (aCompMgr) { return true; }
};

function NSGetModule(aCompMgr, aFileSpec)
{
  return gRegTestCoreComponentModule;
}
