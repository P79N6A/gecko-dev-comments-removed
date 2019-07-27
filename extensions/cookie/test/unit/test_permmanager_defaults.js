



const TEST_ORIGIN = "example.org";
const TEST_PERMISSION = "test-permission";

function run_test() {
  run_next_test();
}

add_task(function* do_test() {
  
  do_get_profile();

  
  let file = do_get_tempdir();
  file.append("test_default_permissions");

  
  let ostream = Cc["@mozilla.org/network/file-output-stream;1"].
                createInstance(Ci.nsIFileOutputStream);
  ostream.init(file, -1, 0666, 0);
  let conv = Cc["@mozilla.org/intl/converter-output-stream;1"].
             createInstance(Ci.nsIConverterOutputStream);
  conv.init(ostream, "UTF-8", 0, 0);

  conv.writeString("# this is a comment\n");
  conv.writeString("\n"); 
  conv.writeString("host\t" + TEST_PERMISSION + "\t1\t" + TEST_ORIGIN + "\n");
  ostream.close();

  
  Services.prefs.setCharPref("permissions.manager.defaultsUrl", "file://" + file.path);

  
  let pm = Cc["@mozilla.org/permissionmanager;1"].
           getService(Ci.nsIPermissionManager);

  
  let permURI = NetUtil.newURI("http://" + TEST_ORIGIN);
  let principal = Services.scriptSecurityManager.getNoAppCodebasePrincipal(permURI);

  do_check_eq(Ci.nsIPermissionManager.ALLOW_ACTION,
              pm.testPermissionFromPrincipal(principal, TEST_PERMISSION));

  
  do_check_eq(Ci.nsIPermissionManager.ALLOW_ACTION, findCapabilityViaEnum());
  
  yield checkCapabilityViaDB(null);

  
  pm.removeAll();

  do_check_eq(Ci.nsIPermissionManager.ALLOW_ACTION,
              pm.testPermissionFromPrincipal(principal, TEST_PERMISSION));

  
  
  pm.removeFromPrincipal(principal, TEST_PERMISSION);
  do_check_eq(Ci.nsIPermissionManager.UNKNOWN_ACTION,
              pm.testPermissionFromPrincipal(principal, TEST_PERMISSION));
  
  yield checkCapabilityViaDB(Ci.nsIPermissionManager.UNKNOWN_ACTION);
  
  do_check_eq(null, findCapabilityViaEnum());

  
  pm.removeAll();

  do_check_eq(Ci.nsIPermissionManager.ALLOW_ACTION,
              pm.testPermissionFromPrincipal(principal, TEST_PERMISSION));
  
  do_check_eq(Ci.nsIPermissionManager.ALLOW_ACTION, findCapabilityViaEnum());

  
  pm.addFromPrincipal(principal, TEST_PERMISSION, Ci.nsIPermissionManager.DENY_ACTION);

  
  do_check_eq(Ci.nsIPermissionManager.DENY_ACTION,
              pm.testPermissionFromPrincipal(principal, TEST_PERMISSION));
  do_check_eq(Ci.nsIPermissionManager.DENY_ACTION, findCapabilityViaEnum());
  yield checkCapabilityViaDB(Ci.nsIPermissionManager.DENY_ACTION);

  
  
  pm.addFromPrincipal(principal, TEST_PERMISSION, Ci.nsIPermissionManager.PROMPT_ACTION);

  
  do_check_eq(Ci.nsIPermissionManager.PROMPT_ACTION,
              pm.testPermissionFromPrincipal(principal, TEST_PERMISSION));
  do_check_eq(Ci.nsIPermissionManager.PROMPT_ACTION, findCapabilityViaEnum());
  yield checkCapabilityViaDB(Ci.nsIPermissionManager.PROMPT_ACTION);

  
  file.remove(false);
});




function findCapabilityViaEnum(host = TEST_ORIGIN, type = TEST_PERMISSION) {
  let result = undefined;
  let e = Services.perms.enumerator;
  while (e.hasMoreElements()) {
    let perm = e.getNext().QueryInterface(Ci.nsIPermission);
    if (perm.host == host &&
        perm.type == type) {
      if (result !== undefined) {
        
        do_throw("enumerator found multiple entries");
      }
      result = perm.capability;
    }
  }
  return result || null;
}






function checkCapabilityViaDB(expected, host = TEST_ORIGIN, type = TEST_PERMISSION) {
  let deferred = Promise.defer();
  let count = 0;
  let max = 20;
  let do_check = () => {
    let got = findCapabilityViaDB(host, type);
    if (got == expected) {
      
      do_check_eq(got, expected, "The database has the expected value");
      deferred.resolve();
      return;
    }
    
    if (count++ == max) {
      
      do_check_eq(got, expected, "The database wasn't updated with the expected value");
      deferred.resolve();
      return;
    }
    
    do_timeout(100, do_check);
  }
  do_check();
  return deferred.promise;
}




function findCapabilityViaDB(host = TEST_ORIGIN, type = TEST_PERMISSION) {
  let file = Services.dirsvc.get("ProfD", Ci.nsIFile);
  file.append("permissions.sqlite");

  let storage = Cc["@mozilla.org/storage/service;1"]
                  .getService(Ci.mozIStorageService);

  let connection = storage.openDatabase(file);

  let query = connection.createStatement(
      "SELECT permission FROM moz_hosts WHERE host = :host AND type = :type");
  query.bindByName("host", host);
  query.bindByName("type", type);

  if (!query.executeStep()) {
    
    return null;
  }
  let result = query.getInt32(0);
  if (query.executeStep()) {
    
    do_throw("More than 1 row found!")
  }
  return result;
}
