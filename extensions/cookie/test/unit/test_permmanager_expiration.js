const Cc = Components.classes;
const Ci = Components.interfaces;

  
  var dir = do_get_profile();

  
  var pm = Cc["@mozilla.org/permissionmanager;1"]
           .getService(Ci.nsIPermissionManager);

  var ios = Cc["@mozilla.org/network/io-service;1"]
            .getService(Ci.nsIIOService);
  var permURI = ios.newURI("http://example.com", null, null);

function run_test() {

  
  pm.add(permURI, "test/expiration-perm-exp", 1, pm.EXPIRE_TIME, (new Date()).getTime());

  
  pm.add(permURI, "test/expiration-perm-exp2", 1, pm.EXPIRE_TIME, (new Date()).getTime() + 100);

  
  pm.add(permURI, "test/expiration-perm-exp3", 1, pm.EXPIRE_TIME, (new Date()).getTime() + 10000);

  
  pm.add(permURI, "test/expiration-perm-nexp", 1, pm.EXPIRE_NEVER, 0);

  
  do_check_eq(1, pm.testPermission(permURI, "test/expiration-perm-exp3"));
  do_check_eq(1, pm.testPermission(permURI, "test/expiration-perm-nexp"));

  
  do_test_pending();
  do_timeout(10, verifyFirstExpiration);

  
  do_test_pending();
  do_timeout(200, verifyExpiration);

  
  do_test_pending();
  do_timeout(300, end_test);
}

function verifyFirstExpiration() { 
  do_check_eq(0, pm.testPermission(permURI, "test/expiration-perm-exp"));
  do_test_finished();
}

function verifyExpiration() { 
  do_check_eq(0, pm.testPermission(permURI, "test/expiration-perm-exp2")); 
  do_test_finished();
}

function end_test() {
  
  pm.removeAll();
  do_test_finished();
}
