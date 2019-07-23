









































function run_test() {
  

  
  
  if (!("@mozilla.org/windows-registry-key;1" in Components.classes))
    return;

  

  Cu.import("resource://gre/modules/XPCOMUtils.jsm");

  











  function MockWindowsRegKey(aWrappedObject) {
    this._wrappedObject = aWrappedObject;

    
    function makeForwardingFunction(functionName) {
      return function() {
        return aWrappedObject[functionName].apply(aWrappedObject, arguments);
      }
    }

    
    for (var propertyName in aWrappedObject) {
      if (!(propertyName in this)) {
        if (typeof aWrappedObject[propertyName] == "function") {
          this[propertyName] = makeForwardingFunction(propertyName);
        } else {
          this[propertyName] = aWrappedObject[propertyName];
        }
      }
    }
  }

  MockWindowsRegKey.prototype = {
    

    QueryInterface: XPCOMUtils.generateQI([Ci.nsIWindowsRegKey]),

    

    open: function(aRootKey, aRelPath, aMode) {
      
      this._rootKey = aRootKey;
      this._relPath = aRelPath;

      
      return this._wrappedObject.open(aRootKey, aRelPath, aMode);
    },

    openChild: function(aRelPath, aMode) {
      
      var innerKey = this._wrappedObject.openChild(aRelPath, aMode);
      var key = new MockWindowsRegKey(innerKey);

      
      key._rootKey = this._rootKey;
      key._relPath = this._relPath + aRelPath;
      return key;
    },

    createChild: function(aRelPath, aMode) {
      
      var innerKey = this._wrappedObject.createChild(aRelPath, aMode);
      var key = new MockWindowsRegKey(innerKey);

      
      key._rootKey = this._rootKey;
      key._relPath = this._relPath + aRelPath;
      return key;
    },

    get childCount() {
      return this._wrappedObject.childCount;
    },

    get valueCount() {
      return this._wrappedObject.valueCount;
    },

    readStringValue: function(aName) {
      
      if (this._rootKey == Ci.nsIWindowsRegKey.ROOT_KEY_CLASSES_ROOT &&
          this._relPath.toLowerCase() == ".txt" &&
          aName.toLowerCase() == "content type") {
        return "";
      }

      
      return this._wrappedObject.readStringValue(aName);
    }
  };

  

  var componentRegistrar = Components.manager.
                           QueryInterface(Ci.nsIComponentRegistrar);

  var originalWindowsRegKeyFactory;
  var mockWindowsRegKeyFactory;

  const kWindowsRegKeyCID = "{a53bc624-d577-4839-b8ec-bb5040a52ff4}";
  const kWindowsRegKeyContractID = "@mozilla.org/windows-registry-key;1";
  const kWindowsRegKeyClassName = "nsWindowsRegKey";

  function registerMockWindowsRegKeyFactory() {
    mockWindowsRegKeyFactory = {
      createInstance: function(aOuter, aIid) {
        if (aOuter != null)
          throw Cr.NS_ERROR_NO_AGGREGATION;

        var innerKey = originalWindowsRegKeyFactory.createInstance(null, aIid);
        var key = new MockWindowsRegKey(innerKey);

        return key.QueryInterface(aIid);
      }
    };

    
    originalWindowsRegKeyFactory = Components.manager.getClassObject(
                                   Cc[kWindowsRegKeyContractID], Ci.nsIFactory);

    
    componentRegistrar.registerFactory(
      Components.ID(kWindowsRegKeyCID),
      "Mock Windows Registry Key Implementation",
      kWindowsRegKeyContractID,
      mockWindowsRegKeyFactory
    );
  }

  function unregisterMockWindowsRegKeyFactory() {
    
    componentRegistrar.unregisterFactory(
      Components.ID(kWindowsRegKeyCID),
      mockWindowsRegKeyFactory
    );

    
    componentRegistrar.registerFactory(
      Components.ID(kWindowsRegKeyCID),
      kWindowsRegKeyClassName,
      kWindowsRegKeyContractID,
      originalWindowsRegKeyFactory
    );
  }

  

  
  registerMockWindowsRegKeyFactory();
  try {
    
    
    var type = Cc["@mozilla.org/mime;1"].
               getService(Ci.nsIMIMEService).
               getTypeFromExtension(".txt");
  } catch (e if (e instanceof Ci.nsIException &&
                 e.result == Cr.NS_ERROR_NOT_AVAILABLE)) {
    
  } finally {
    
    unregisterMockWindowsRegKeyFactory();
  }
}
