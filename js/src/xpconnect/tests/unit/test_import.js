





































 
function run_test() {   
  var scope = {};
  Components.utils.import("resource://gre/modules/XPCOMUtils.jsm", scope);
  do_check_eq(typeof(scope.XPCOMUtils), "object");
  do_check_eq(typeof(scope.XPCOMUtils.generateModule), "function");
  
  
  
  var module = Components.utils.import("resource://gre/modules/XPCOMUtils.jsm",
                                       null);
  do_check_eq(typeof(XPCOMUtils), "undefined");
  do_check_eq(typeof(module), "object");
  do_check_eq(typeof(module.XPCOMUtils), "object");
  do_check_eq(typeof(module.XPCOMUtils.generateModule), "function");
  do_check_true(scope.XPCOMUtils == module.XPCOMUtils);

  
  do_check_eq(typeof(Components.utils.import), "function");
  Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
  do_check_eq(typeof(XPCOMUtils), "object");
  do_check_eq(typeof(XPCOMUtils.generateModule), "function");
  
  
  var scope2 = {};
  Components.utils.import("resource://gre/modules/XPCOMUtils.jsm", scope2);
  do_check_eq(typeof(scope2.XPCOMUtils), "object");
  do_check_eq(typeof(scope2.XPCOMUtils.generateModule), "function");
  
  do_check_true(scope2.XPCOMUtils == scope.XPCOMUtils);

  
  var res = Components.classes["@mozilla.org/network/protocol;1?name=resource"]
                      .getService(Components.interfaces.nsIResProtocolHandler);
  var resURI = res.newURI("resource://gre/modules/XPCOMUtils.jsm", null, null);
  dump("resURI: " + resURI + "\n");
  var filePath = res.resolveURI(resURI);
  do_check_eq(filePath.indexOf("file://"), 0);
  var scope3 = {};
  Components.utils.import(filePath, scope3);
  do_check_eq(typeof(scope3.XPCOMUtils), "object");
  do_check_eq(typeof(scope3.XPCOMUtils.generateModule), "function");
  
  do_check_true(scope3.XPCOMUtils == scope.XPCOMUtils);

  
  var didThrow = false;
  try {
      Components.utils.import("resource://gre/modules/XPCOMUtils.jsm", "wrong");
  } catch (ex) {
      print("exception (expected): " + ex);
      didThrow = true;
  }
  do_check_true(didThrow);
 
  
  do_load_module("component_import.js");
  const contractID = "@mozilla.org/tests/module-importer;";
  do_check_true((contractID + "1") in Components.classes);
  var foo = Components.classes[contractID + "1"]
                      .createInstance(Components.interfaces.nsIClassInfo);
  do_check_true(Boolean(foo));
  do_check_true(foo.contractID == contractID + "1");
  
  
  
  

  
  
  
  
  var interfaces = foo.getInterfaces({});
  do_check_eq(interfaces, Components.interfaces.nsIClassInfo.number);

  
  const cid = "{6b933fe6-6eba-4622-ac86-e4f654f1b474}";
  do_check_true(cid in Components.classesByID);
  foo = Components.classesByID[cid]
                  .createInstance(Components.interfaces.nsIClassInfo);
  do_check_true(foo.contractID == contractID + "1");

  
  do_check_true((contractID + "2") in Components.classes);
  var bar = Components.classes[contractID + "2"]
                      .createInstance(Components.interfaces.nsIClassInfo);
  do_check_true(Boolean(bar));
  do_check_true(bar.contractID == contractID + "2");
}
