






































var gRegTestCoreDeferredComponent =
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

var gRegTestCoreDeferredComponentModule =
{
  kClassName: "RegTestCoreDeferredComponent",
  kClassID: Components.ID("{d04d1298-6dac-459b-a13b-bcab235730a0}"),
  kContractID: "@mozilla.org/RegTestServiceB;1",
  kICompReg: Components.interfaces.nsIComponentRegistrar,

  mIsFirstTime: true,

  
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

    return gRegTestCoreDeferredComponent.QueryInterface(aIID);
  },

  registerSelf: function (aCompMgr, aFileSpec, aLocation, aType)
  {
    if (this.mIsFirstTime)
    {
      this.mIsFirstTime = false;
      throw Components.results.NS_ERROR_FACTORY_REGISTER_AGAIN;
    }

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
  return gRegTestCoreDeferredComponentModule;
}
