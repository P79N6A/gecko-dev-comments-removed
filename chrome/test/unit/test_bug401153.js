





































var MANIFESTS = [
  do_get_file("chrome/test/unit/data/test_bug401153.manifest")
];

registerManifests(MANIFESTS);

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

var rph = gIOS.getProtocolHandler("resource")
              .QueryInterface(Ci.nsIResProtocolHandler);

function test_succeeded_mapping(namespace, target)
{
  try {
    do_check_true(rph.hasSubstitution(namespace));
    var thistarget = target + "/test.js";
    var uri = gIOS.newURI("resource://" + namespace + "/test.js",
                          null, null);
    do_check_eq(rph.resolveURI(uri), thistarget);
  }
  catch (ex) {
    do_throw(namespace);
  }
}

function test_failed_mapping(namespace)
{
  do_check_false(rph.hasSubstitution(namespace));
}

function run_test()
{
  var data = gIOS.newFileURI(do_get_file("chrome/test/unit/data")).spec;
  test_succeeded_mapping("test1", data + "test1");
  test_succeeded_mapping("test2", "http://www.mozilla.org");
  test_succeeded_mapping("test3", "jar:" + data + "test3.jar!/resources");
  test_failed_mapping("test4");
  test_succeeded_mapping("test5", data + "test5");
}
