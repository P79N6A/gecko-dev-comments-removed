





































 
function run_test() {   
  var scope = {};
  Components.utils.import("rel:XPCOMUtils.jsm", scope);
  do_check_eq(typeof(scope.XPCOMUtils), "object");
  do_check_eq(typeof(scope.XPCOMUtils.generateFactory), "function");
  
  
  do_check_eq(typeof(Components.utils.import), "function");
  Components.utils.import("rel:XPCOMUtils.jsm");
  do_check_eq(typeof(XPCOMUtils), "object");
  do_check_eq(typeof(XPCOMUtils.generateFactory), "function");
  
  
  var scope2 = {};
  Components.utils.import("rel:XPCOMUtils.jsm", scope2);
  do_check_eq(typeof(scope2.XPCOMUtils), "object");
  do_check_eq(typeof(scope2.XPCOMUtils.generateFactory), "function");
  
  do_check_true(scope2.XPCOMUtils == scope.XPCOMUtils);

  
  var didThrow = false;
  try {
      Components.utils.import("rel:XPCOMUtils.jsm", "wrong");
  } catch (ex) {
      print("ex: " + ex);
      didThrow = true;
  }
  do_check_true(didThrow);
 
  
  do_load_module("/js/src/xpconnect/tests/unit/component_import.js");
  const contractID = "@mozilla.org/tests/module-importer;";
  do_check_true((contractID + "1") in Components.classes);
  var foo = Components.classes[contractID + "1"]
                      .createInstance(Components.interfaces.nsIClassInfo);
  do_check_true(Boolean(foo));
  do_check_true(foo.contractID == contractID + "1");

  
  do_check_true((contractID + "2") in Components.classes);
  var bar = Components.classes[contractID + "2"]
                      .createInstance(Components.interfaces.nsIClassInfo);
  do_check_true(Boolean(bar));
  do_check_true(bar.contractID == contractID + "2");
}
