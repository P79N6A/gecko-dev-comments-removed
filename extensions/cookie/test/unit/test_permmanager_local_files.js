




function getPrincipalFromURIString(uriStr)
{
  return Services.scriptSecurityManager.getNoAppCodebasePrincipal(NetUtil.newURI(uriStr));
}

function run_test() {
  let pm = Services.permissions;

  
  let principal = getPrincipalFromURIString("file:///foo/bar");
  pm.addFromPrincipal(principal, "test/local-files", pm.ALLOW_ACTION, 0, 0);
  do_check_eq(pm.testPermissionFromPrincipal(principal, "test/local-files"), pm.ALLOW_ACTION);

  
  let witnessPrincipal = getPrincipalFromURIString("file:///bar/foo");
  do_check_eq(pm.testPermissionFromPrincipal(witnessPrincipal, "test/local-files"), pm.UNKNOWN_ACTION);

  
  let rootPrincipal = getPrincipalFromURIString("file:///");
  pm.addFromPrincipal(rootPrincipal, "test/local-files", pm.ALLOW_ACTION, 0, 0);
  do_check_eq(pm.testPermissionFromPrincipal(witnessPrincipal, "test/local-files"), pm.UNKNOWN_ACTION);

  
  let schemeRootPrincipal = getPrincipalFromURIString("file://");
  pm.addFromPrincipal(schemeRootPrincipal, "test/local-files", pm.ALLOW_ACTION, 0, 0);
  do_check_eq(pm.testPermissionFromPrincipal(witnessPrincipal, "test/local-files"), pm.UNKNOWN_ACTION);

  
  let fileInDirPrincipal = getPrincipalFromURIString("file:///foo/bar/foobar.txt");
  do_check_eq(pm.testPermissionFromPrincipal(fileInDirPrincipal, "test/local-files"), pm.UNKNOWN_ACTION);

  
  pm.removeFromPrincipal(principal, "test/local-files");
  do_check_eq(pm.testPermissionFromPrincipal(principal, "test/local-files"), pm.UNKNOWN_ACTION);
  do_check_eq(pm.testPermissionFromPrincipal(witnessPrincipal, "test/local-files"), pm.UNKNOWN_ACTION);
  do_check_eq(pm.testPermissionFromPrincipal(fileInDirPrincipal, "test/local-files"), pm.UNKNOWN_ACTION);

  
  pm.addFromPrincipal(getPrincipalFromURIString("http://<file>"), "test/local-files", pm.ALLOW_ACTION, 0, 0);
  do_check_eq(pm.testPermissionFromPrincipal(principal, "test/local-files"), pm.ALLOW_ACTION);
  do_check_eq(pm.testPermissionFromPrincipal(witnessPrincipal, "test/local-files"), pm.ALLOW_ACTION);
  do_check_eq(pm.testPermissionFromPrincipal(fileInDirPrincipal, "test/local-files"), pm.ALLOW_ACTION);
}