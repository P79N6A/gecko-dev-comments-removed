





































const Cc = Components.classes;
const Ci = Components.interfaces;

const ID = "bug486195@tests.mozilla.org";
const REGKEY = "SOFTWARE\\Mozilla\\XPCShell\\Extensions";

function run_test()
{
  if (!("nsIWindowsRegKey" in Ci))
    return;

  var extension = do_get_file("data/test_bug486195");

  Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

  

  



  function MockWindowsRegKey() {
  }

  MockWindowsRegKey.prototype = {
    

    QueryInterface: XPCOMUtils.generateQI([Ci.nsIWindowsRegKey]),

    

    open: function(aRootKey, aRelPath, aMode) {
      if (aRootKey != Ci.nsIWindowsRegKey.ROOT_KEY_LOCAL_MACHINE)
        throw Components.results.NS_ERROR_FAILURE;
      if (aRelPath != REGKEY)
        throw Components.results.NS_ERROR_FAILURE;
    },

    close: function() {
    },

    get valueCount() {
      return 1;
    },

    getValueName: function(aIndex) {
      if (aIndex == 0)
        return ID;
      throw Components.results.NS_ERROR_FAILURE;
    },

    readStringValue: function(aName) {
      if (aName == ID)
        return extension.path;
      throw Components.results.NS_ERROR_FAILURE;
    }
  };

  var factory = {
    createInstance: function(aOuter, aIid) {
      if (aOuter != null)
        throw Cr.NS_ERROR_NO_AGGREGATION;

      var key = new MockWindowsRegKey();
      return key.QueryInterface(aIid);
    }
  };

  Components.manager.QueryInterface(Ci.nsIComponentRegistrar)
            .registerFactory(Components.ID("{0478de5b-0f38-4edb-851d-4c99f1ed8eba}"),
                             "Mock Windows Registry Implementation",
                             "@mozilla.org/windows-registry-key;1", factory);

  
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1");

  
  startupEM();
  var addon = gEM.getItemForID(ID);
  do_check_neq(addon, null);
  var installLocation = gEM.getInstallLocation(ID);
  var path = installLocation.getItemLocation(ID);
  do_check_eq(extension.path, path.path);
  path.append("install.rdf");
  path = installLocation.getItemLocation(ID);
  do_check_eq(extension.path, path.path);

  shutdownEM();

  Components.manager.unregisterFactory(Components.ID("{0478de5b-0f38-4edb-851d-4c99f1ed8eba}"),
                                       factory);
}
