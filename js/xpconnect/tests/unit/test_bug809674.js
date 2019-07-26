



const Cc = Components.classes;
const Ci = Components.interfaces;

function run_test() {

  
  Components.manager.autoRegister(do_get_file('../components/js/xpctest.manifest'));

  
  test_property_throws("@mozilla.org/js/xpc/test/js/Bug809674;1");
}

function test_property_throws(contractid) {

  
  var o = Cc[contractid].createInstance(Ci["nsIXPCTestBug809674"]);

  
  try {
    o.jsvalProperty;
    do_check_true(false);
  } catch (e) {
    do_check_true(true);
    
    
    
    
    todo_check_true(/implicit_jscontext/.test(e))
  }

}
