





































 
function run_test() {
  const Ci = Components.interfaces; 
  const ioService = Components.classes["@mozilla.org/network/io-service;1"]
                              .getService(Ci.nsIIOService);
  const resProt = ioService.getProtocolHandler("resource")
                           .QueryInterface(Ci.nsIResProtocolHandler);

  var curdir = do_get_file("js/src/xpconnect/tests/unit");
  var curURI = ioService.newFileURI(curdir);
  resProt.setSubstitution("test", curURI);

  var scope = {};
  Components.utils.import("resource://test/recursive_importA.jsm", scope);

  
  do_check_true(scope.foo() == "foo");

  
  do_check_true(scope.bar.baz() == "baz");

  
  do_check_true(scope.bar.qux.foo() == "foo");
}
