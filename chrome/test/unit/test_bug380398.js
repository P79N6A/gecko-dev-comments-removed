





































const XULAPPINFO_CONTRACTID = "@mozilla.org/xre/app-info;1";
const XULAPPINFO_CID = Components.ID("{48a4e946-1f9f-4224-b4b0-9a54183cb81e}");

const NS_CHROME_MANIFESTS_FILE_LIST = "ChromeML";

const Cc = Components.classes;
const Ci = Components.interfaces;

var MANIFESTS = [
  do_get_file("chrome/test/unit/data/test_bug380398.manifest")
];

function ArrayEnumerator(array)
{
  this.array = array;
}

ArrayEnumerator.prototype = {
  pos: 0,
  array: null,
  
  hasMoreElements: function() {
    return this.pos < this.array.length;
  },
  
  getNext: function() {
    if (this.pos < this.array.length)
      return this.array[this.pos++];
    throw Components.results.NS_ERROR_FAILURE;
  },

  QueryInterface: function(iid) {
    if (iid.equals(Ci.nsISimpleEnumerator)
     || iid.equals(Ci.nsISupports))
      return this;

    throw Components.results.NS_ERROR_NO_INTERFACE;
  }
};

var ChromeProvider = {
  getFile: function(prop, persistent) {
    throw Components.results.NS_ERROR_FAILURE
  },
  
  getFiles: function(prop) {
    if (prop == NS_CHROME_MANIFESTS_FILE_LIST) {
      return new ArrayEnumerator(MANIFESTS);
    }
    throw Components.results.NS_ERROR_FAILURE
  },
  
  QueryInterface: function(iid) {
    if (iid.equals(Ci.nsIDirectoryServiceProvider)
     || iid.equals(Ci.nsIDirectoryServiceProvider2)
     || iid.equals(Ci.nsISupports))
      return this;

    throw Components.results.NS_ERROR_NO_INTERFACE;
  }
};
var dirSvc = Cc["@mozilla.org/file/directory_service;1"]
              .getService(Ci.nsIDirectoryService);
dirSvc.registerProvider(ChromeProvider);

var XULAppInfo = {
  vendor: "Mozilla",
  name: "XPCShell",
  ID: "{39885e5f-f6b4-4e2a-87e5-6259ecf79011}",
  version: "5",
  appBuildID: "2007010101",
  platformVersion: "1.9",
  platformBuildID: "2007010101",
  inSafeMode: false,
  logConsoleErrors: true,
  OS: "XPCShell",
  XPCOMABI: "noarch-spidermonkey",
  
  QueryInterface: function QueryInterface(iid) {
    if (iid.equals(Ci.nsIXULAppInfo)
     || iid.equals(Ci.nsIXULRuntime)
     || iid.equals(Ci.nsISupports))
      return this;
  
    throw Components.results.NS_ERROR_NO_INTERFACE;
  }
};

var XULAppInfoFactory = {
  createInstance: function (outer, iid) {
    if (outer != null)
      throw Components.results.NS_ERROR_NO_AGGREGATION;
    return XULAppInfo.QueryInterface(iid);
  }
};
var registrar = Components.manager.QueryInterface(Ci.nsIComponentRegistrar);
registrar.registerFactory(XULAPPINFO_CID, "XULAppInfo",
                          XULAPPINFO_CONTRACTID, XULAppInfoFactory);

var gIOS = Cc["@mozilla.org/network/io-service;1"]
            .getService(Ci.nsIIOService);
var chromeReg = Cc["@mozilla.org/chrome/chrome-registry;1"]
                 .getService(Ci.nsIChromeRegistry);
chromeReg.checkForNewChrome();

var target = gIOS.newFileURI(do_get_file("chrome/test/unit/data"));
target = target.spec + "test/test.xul";

function test_succeeded_mapping(namespace)
{
  var uri = gIOS.newURI("chrome://" + namespace + "/content/test.xul",
                        null, null);
  try {
    var result = chromeReg.convertChromeURL(uri);
    do_check_eq(result.spec, target);
  }
  catch (ex) {
    do_throw(namespace);
  }
}

function test_failed_mapping(namespace)
{
  var uri = gIOS.newURI("chrome://" + namespace + "/content/test.xul",
                        null, null);
  try {
    var result = chromeReg.convertChromeURL(uri);
    do_throw(namespace);
  }
  catch (ex) {
  }
}

function run_test()
{
  test_succeeded_mapping("test1");
  test_succeeded_mapping("test2");
  test_succeeded_mapping("test3");
  test_succeeded_mapping("test4");
  test_succeeded_mapping("test5");
  test_failed_mapping("test6");
  test_failed_mapping("test7");
  test_failed_mapping("test8");
  test_failed_mapping("test9");
  test_failed_mapping("test10");
  test_failed_mapping("test11");
}
