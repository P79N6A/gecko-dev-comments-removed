


function getPrincipalFromURI(uri) {
  return Cc["@mozilla.org/scriptsecuritymanager;1"]
           .getService(Ci.nsIScriptSecurityManager)
           .getNoAppCodebasePrincipal(NetUtil.newURI(uri));
}

function run_test() {
  var pm = Cc["@mozilla.org/permissionmanager;1"].
           getService(Ci.nsIPermissionManager);

  
  let sub1Principal = getPrincipalFromURI("http://sub1.example.com");
  pm.addFromPrincipal(sub1Principal, "test/subdomains", pm.ALLOW_ACTION, 0, 0);
  do_check_eq(pm.testPermissionFromPrincipal(sub1Principal, "test/subdomains"), pm.ALLOW_ACTION);

  
  let subsubPrincipal = getPrincipalFromURI("http://sub.sub1.example.com");
  do_check_eq(pm.testPermissionFromPrincipal(subsubPrincipal, "test/subdomains"), pm.ALLOW_ACTION);

  
  let sub2Principal = getPrincipalFromURI("http://sub2.example.com");
  do_check_eq(pm.testPermissionFromPrincipal(sub2Principal, "test/subdomains"), pm.UNKNOWN_ACTION);

  
  pm.removeFromPrincipal(sub1Principal, "test/subdomains");
  do_check_eq(pm.testPermissionFromPrincipal(sub1Principal, "test/subdomains"), pm.UNKNOWN_ACTION);

  
  let mainPrincipal = getPrincipalFromURI("http://example.com");
  pm.addFromPrincipal(mainPrincipal, "test/subdomains", pm.ALLOW_ACTION, 0, 0);
  do_check_eq(pm.testPermissionFromPrincipal(mainPrincipal, "test/subdomains"), pm.ALLOW_ACTION);

  
  do_check_eq(pm.testPermissionFromPrincipal(sub1Principal, "test/subdomains"), pm.ALLOW_ACTION);
  do_check_eq(pm.testPermissionFromPrincipal(sub2Principal, "test/subdomains"), pm.ALLOW_ACTION);
  do_check_eq(pm.testPermissionFromPrincipal(subsubPrincipal, "test/subdomains"), pm.ALLOW_ACTION);

  
  pm.removeFromPrincipal(mainPrincipal, "test/subdomains");
  do_check_eq(pm.testPermissionFromPrincipal(mainPrincipal, "test/subdomains"), pm.UNKNOWN_ACTION);
  do_check_eq(pm.testPermissionFromPrincipal(sub1Principal, "test/subdomains"), pm.UNKNOWN_ACTION);
  do_check_eq(pm.testPermissionFromPrincipal(sub2Principal, "test/subdomains"), pm.UNKNOWN_ACTION);
  do_check_eq(pm.testPermissionFromPrincipal(subsubPrincipal, "test/subdomains"), pm.UNKNOWN_ACTION);

  
  let crazyPrincipal = getPrincipalFromURI("http://com");
  pm.addFromPrincipal(crazyPrincipal, "test/subdomains", pm.ALLOW_ACTION, 0, 0);
  do_check_eq(pm.testPermissionFromPrincipal(crazyPrincipal, "test/subdomains"),  pm.ALLOW_ACTION);
  do_check_eq(pm.testPermissionFromPrincipal(mainPrincipal, "test/subdomains"),   pm.UNKNOWN_ACTION);
  do_check_eq(pm.testPermissionFromPrincipal(sub1Principal, "test/subdomains"),   pm.UNKNOWN_ACTION);
  do_check_eq(pm.testPermissionFromPrincipal(sub2Principal, "test/subdomains"),   pm.UNKNOWN_ACTION);
  do_check_eq(pm.testPermissionFromPrincipal(subsubPrincipal, "test/subdomains"), pm.UNKNOWN_ACTION);
}