



const TEST_ORIGIN = NetUtil.newURI("http://example.org");
const TEST_ORIGIN_HTTPS = NetUtil.newURI("https://example.org");
const TEST_ORIGIN_2 = NetUtil.newURI("http://example.com");
const TEST_ORIGIN_3 = NetUtil.newURI("https://example2.com:8080");
const TEST_PERMISSION = "test-permission";
Components.utils.import("resource://gre/modules/Promise.jsm");

function promiseTimeout(delay) {
  let deferred = Promise.defer();
  do_timeout(delay, deferred.resolve);
  return deferred.promise;
}

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
  conv.writeString("host\t" + TEST_PERMISSION + "\t1\t" + TEST_ORIGIN.host + "\n");
  conv.writeString("host\t" + TEST_PERMISSION + "\t1\t" + TEST_ORIGIN_2.host + "\n");
  conv.writeString("origin\t" + TEST_PERMISSION + "\t1\t" + TEST_ORIGIN_3.spec + "\n");
  conv.writeString("origin\t" + TEST_PERMISSION + "\t1\t" + TEST_ORIGIN.spec + "!appId=1000&inBrowser=1\n");
  ostream.close();

  
  Services.prefs.setCharPref("permissions.manager.defaultsUrl", "file://" + file.path);

  
  let pm = Cc["@mozilla.org/permissionmanager;1"].
           getService(Ci.nsIPermissionManager);

  
  let principal = Services.scriptSecurityManager.getNoAppCodebasePrincipal(TEST_ORIGIN);
  let principalHttps = Services.scriptSecurityManager.getNoAppCodebasePrincipal(TEST_ORIGIN_HTTPS);
  let principal2 = Services.scriptSecurityManager.getNoAppCodebasePrincipal(TEST_ORIGIN_2);
  let principal3 = Services.scriptSecurityManager.getNoAppCodebasePrincipal(TEST_ORIGIN_3);
  let principal4 = Services.scriptSecurityManager.getAppCodebasePrincipal(TEST_ORIGIN, 1000, true);
  let principal5 = Services.scriptSecurityManager.getAppCodebasePrincipal(TEST_ORIGIN_3, 1000, true);

  do_check_eq(Ci.nsIPermissionManager.ALLOW_ACTION,
              pm.testPermissionFromPrincipal(principal, TEST_PERMISSION));
  do_check_eq(Ci.nsIPermissionManager.ALLOW_ACTION,
              pm.testPermissionFromPrincipal(principalHttps, TEST_PERMISSION));
  do_check_eq(Ci.nsIPermissionManager.ALLOW_ACTION,
              pm.testPermissionFromPrincipal(principal3, TEST_PERMISSION));
  do_check_eq(Ci.nsIPermissionManager.ALLOW_ACTION,
              pm.testPermissionFromPrincipal(principal4, TEST_PERMISSION));

  
  do_check_eq(Ci.nsIPermissionManager.UNKNOWN_ACTION,
              pm.testPermissionFromPrincipal(principal5, TEST_PERMISSION));

  
  do_check_eq(Ci.nsIPermissionManager.ALLOW_ACTION, findCapabilityViaEnum(TEST_ORIGIN));
  do_check_eq(Ci.nsIPermissionManager.ALLOW_ACTION, findCapabilityViaEnum(TEST_ORIGIN_3));

  
  yield checkCapabilityViaDB(null);

  
  pm.removeAll();

  do_check_eq(Ci.nsIPermissionManager.ALLOW_ACTION,
              pm.testPermissionFromPrincipal(principal, TEST_PERMISSION));
  do_check_eq(Ci.nsIPermissionManager.ALLOW_ACTION,
              pm.testPermissionFromPrincipal(principal3, TEST_PERMISSION));
  do_check_eq(Ci.nsIPermissionManager.ALLOW_ACTION,
              pm.testPermissionFromPrincipal(principal4, TEST_PERMISSION));

  
  
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

  
  
  pm.removeAll(); 

  
  do_check_eq(Ci.nsIPermissionManager.ALLOW_ACTION,
              pm.testPermissionFromPrincipal(principal, TEST_PERMISSION));
  do_check_eq(Ci.nsIPermissionManager.ALLOW_ACTION,
              pm.testPermissionFromPrincipal(principal2, TEST_PERMISSION));

  
  
  pm.addFromPrincipal(principal2, TEST_PERMISSION, Ci.nsIPermissionManager.DENY_ACTION);
  do_check_eq(Ci.nsIPermissionManager.DENY_ACTION,
              pm.testPermissionFromPrincipal(principal2, TEST_PERMISSION));
  yield promiseTimeout(20);

  let since = Number(Date.now());
  yield promiseTimeout(20);

  
  
  pm.addFromPrincipal(principal, TEST_PERMISSION, Ci.nsIPermissionManager.DENY_ACTION);
  do_check_eq(Ci.nsIPermissionManager.DENY_ACTION,
              pm.testPermissionFromPrincipal(principal, TEST_PERMISSION));

  
  pm.removeAllSince(since);

  
  
  do_check_eq(Ci.nsIPermissionManager.ALLOW_ACTION,
              pm.testPermissionFromPrincipal(principal, TEST_PERMISSION));

  
  do_check_eq(Ci.nsIPermissionManager.DENY_ACTION,
              pm.testPermissionFromPrincipal(principal2, TEST_PERMISSION));

  
  file.remove(false);
});




function findCapabilityViaEnum(origin = TEST_ORIGIN, type = TEST_PERMISSION) {
  let result = undefined;
  let e = Services.perms.enumerator;
  while (e.hasMoreElements()) {
    let perm = e.getNext().QueryInterface(Ci.nsIPermission);
    if (perm.matchesURI(origin, true) &&
        perm.type == type) {
      if (result !== undefined) {
        
        do_throw("enumerator found multiple entries");
      }
      result = perm.capability;
    }
  }
  return result || null;
}






function checkCapabilityViaDB(expected, origin = TEST_ORIGIN, type = TEST_PERMISSION) {
  let deferred = Promise.defer();
  let count = 0;
  let max = 20;
  let do_check = () => {
    let got = findCapabilityViaDB(origin, type);
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




function findCapabilityViaDB(origin = TEST_ORIGIN, type = TEST_PERMISSION) {
  let principal = Services.scriptSecurityManager.getNoAppCodebasePrincipal(origin);
  let originStr = principal.origin;

  let file = Services.dirsvc.get("ProfD", Ci.nsIFile);
  file.append("permissions.sqlite");

  let storage = Cc["@mozilla.org/storage/service;1"]
                  .getService(Ci.mozIStorageService);

  let connection = storage.openDatabase(file);

  let query = connection.createStatement(
      "SELECT permission FROM moz_hosts WHERE origin = :origin AND type = :type");
  query.bindByName("origin", originStr);
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
