






































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

const kClassID = Components.ID("{56ab1cd4-ac44-4f86-8104-171f8b8f2fc7}");

function NSGetFactory(aClassID) {
  if (!aClassID.equals(kClassID))
    throw Components.results.NS_ERROR_NOT_IMPLEMENTED;

  return gRegTestCoreComponent;
}
