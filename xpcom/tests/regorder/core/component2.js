






































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

const kClassID = Components.ID("{d04d1298-6dac-459b-a13b-bcab235730a0}");

function NSGetFactory(aClassID) {
  if (!aClassID.equals(kClassID))
    throw Components.results.NS_ERROR_NOT_IMPLEMENTED;

  return gRegTestCoreComponent;
}
