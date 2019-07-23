




































 
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
 
}

