




































const SIMPLETEST_CONTRACTID  = "@test.mozilla.org/simple-test;1?impl=js";
const SIMPLETEST_CLASSID     = Components.ID("{4177e257-a0dc-49b9-a774-522a000a49fa}");

function SimpleTest() {
}

SimpleTest.prototype = {
  QueryInterface: function(iid) {
    if (iid.equals(Components.interfaces.nsISimpleTest) ||
        iid.equals(Components.interfaces.nsISupports))
      return this;
    throw Components.results.NS_ERROR_NO_INTERFACE;
  },

  add: function(a, b) {
    dump("add(" + a + "," + b + ") from JS\n");
    return a + b;
  }
};

var Module = {
  _classes: {
    simpletest: {
      classID     : SIMPLETEST_CLASSID,
      contractID  : SIMPLETEST_CONTRACTID,
      className   : "SimpleTest",
      factory     : {
        createInstance: function(delegate, iid) {
          if (delegate)
            throw Components.results.NS_ERROR_NO_AGGREGATION;
          return new SimpleTest().QueryInterface(iid);
        }
      }
    }
  },

  registerSelf: function(compMgr, fileSpec, location, type)
  {
    var reg = compMgr.QueryInterface(
        Components.interfaces.nsIComponentRegistrar);

    for (var key in this._classes) {
      var c = this._classes[key];
      reg.registerFactoryLocation(c.classID, c.className, c.contractID,
                                  fileSpec, location, type);
    }
  },
                                                                                                        
  getClassObject: function(compMgr, cid, iid)
  {
    if (!iid.equals(Components.interfaces.nsIFactory))
      throw Components.results.NS_ERROR_NO_INTERFACE;

    for (var key in this._classes) {
      var c = this._classes[key];
      if (cid.equals(c.classID))
        return c.factory;
    }

    throw Components.results.NS_ERROR_NOT_IMPLEMENTED;
  },
                                                                                                        
  canUnload: function (aComponentManager)
  {
    return true;
  }
};

function NSGetModule(compMgr, fileSpec) {
  return Module;
}
