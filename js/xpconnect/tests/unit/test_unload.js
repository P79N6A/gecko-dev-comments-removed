


 
function run_test() {
  var scope1 = {};
  var global1 = Components.utils.import("resource://gre/modules/NetUtil.jsm", scope1);

  var scope2 = {};
  var global2 = Components.utils.import("resource://gre/modules/NetUtil.jsm", scope2);

  do_check_true(global1 === global2);
  do_check_true(scope1.NetUtil === scope2.NetUtil);

  Components.utils.unload("resource://gre/modules/NetUtil.jsm");

  var scope3 = {};
  var global3 = Components.utils.import("resource://gre/modules/NetUtil.jsm", scope3);

  do_check_false(global1 === global3);
  do_check_false(scope1.NetUtil === scope3.NetUtil);

  
  
  
  try { scope1.NetUtil.newURI("http://www.example.com"); } catch (e) {}
  try { scope3.NetUtil.newURI("http://www.example.com"); } catch (e) {}
}
