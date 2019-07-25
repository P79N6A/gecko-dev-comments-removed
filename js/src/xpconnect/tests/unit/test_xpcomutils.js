










































Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

const Cc = Components.classes;
const Ci = Components.interfaces;





function test_generateQI_string_names()
{
    var x = {
        QueryInterface: XPCOMUtils.generateQI([
            Components.interfaces.nsIClassInfo,
            "nsIDOMNode"
        ])
    };

    try {
        x.QueryInterface(Components.interfaces.nsIClassInfo);
    } catch(e) {
        do_throw("Should QI to nsIClassInfo");
    }
    try {
        x.QueryInterface(Components.interfaces.nsIDOMNode);
    } catch(e) {
        do_throw("Should QI to nsIDOMNode");
    }
    try {
        x.QueryInterface(Components.interfaces.nsIDOMDocument);
        do_throw("QI should not have succeeded!");
    } catch(e) {}
}


function test_defineLazyGetter()
{
    let accessCount = 0;
    let obj = {
      inScope: false
    };
    const TEST_VALUE = "test value";
    XPCOMUtils.defineLazyGetter(obj, "foo", function() {
        accessCount++;
        this.inScope = true;
        return TEST_VALUE;
    });
    do_check_eq(accessCount, 0);

    
    do_check_eq(obj.foo, TEST_VALUE);
    do_check_eq(accessCount, 1);
    do_check_true(obj.inScope);

    
    
    do_check_eq(obj.foo, TEST_VALUE);
    do_check_eq(accessCount, 1);
}


function test_defineLazyServiceGetter()
{
    let obj = { };
    XPCOMUtils.defineLazyServiceGetter(obj, "service",
                                       "@mozilla.org/consoleservice;1",
                                       "nsIConsoleService");
    let service = Cc["@mozilla.org/consoleservice;1"].
                  getService(Ci.nsIConsoleService);

    
    
    for (let prop in obj.service)
        do_check_true(prop in service);
    for (let prop in service)
        do_check_true(prop in obj.service);
}


function test_categoryRegistration()
{
  const CATEGORY_NAME = "test-cat";

  
  let XULAppInfo = {
    vendor: "Mozilla",
    name: "catRegTest",
    ID: "{adb42a9a-0d19-4849-bf4d-627614ca19be}",
    version: "1",
    appBuildID: "2007010101",
    platformVersion: "",
    platformBuildID: "2007010101",
    inSafeMode: false,
    logConsoleErrors: true,
    OS: "XPCShell",
    XPCOMABI: "noarch-spidermonkey",
    QueryInterface: XPCOMUtils.generateQI([
      Ci.nsIXULAppInfo,
      Ci.nsIXULRuntime,
    ])
  };
  let XULAppInfoFactory = {
    createInstance: function (outer, iid) {
      if (outer != null)
        throw Cr.NS_ERROR_NO_AGGREGATION;
      return XULAppInfo.QueryInterface(iid);
    }
  };
  let registrar = Components.manager.QueryInterface(Ci.nsIComponentRegistrar);
  registrar.registerFactory(
    Components.ID("{6372ef9b-0827-4d18-954f-c0974f1a1573}"),
    "XULAppInfo",
    "@mozilla.org/xre/app-info;1",
    XULAppInfoFactory
  );

  
  do_load_module("CatRegistrationComponents.js");

  const EXPECTED_ENTRIES = ["CatAppRegisteredComponent",
                            "CatRegisteredComponent"];

  
  let foundEntriesCount = 0;
  let catMan = Cc["@mozilla.org/categorymanager;1"].
               getService(Ci.nsICategoryManager);
  let entries = catMan.enumerateCategory(CATEGORY_NAME);
  while (entries.hasMoreElements()) {
    foundEntriesCount++;
    let entry = entries.getNext().QueryInterface(Ci.nsISupportsCString).data;
    print("Check the found category entry (" + entry + ")is expected.");  
    do_check_true(EXPECTED_ENTRIES.indexOf(entry) != -1);
  }
  print("Check there are no more or less than expected entries.");
  do_check_eq(foundEntriesCount, EXPECTED_ENTRIES.length);
}





let tests = [
    test_generateQI_string_names,
    test_defineLazyGetter,
    test_defineLazyServiceGetter,
    test_categoryRegistration,
];

function run_test()
{
    tests.forEach(function(test) {
        print("Running next test: " + test.name);
        test();
    });
}
