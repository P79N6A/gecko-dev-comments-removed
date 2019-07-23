






































var gRegTestExtComponent =
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

var gRegTestExtComponentModule =
{
  kClassName: "RegTestExtComponent",
  kClassID: Components.ID("{fe64efb7-c5ab-41a6-b639-e6c0f483181e}"),
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

    return gRegTestExtComponent.QueryInterface(aIID);
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
  return gRegTestExtComponentModule;
}
