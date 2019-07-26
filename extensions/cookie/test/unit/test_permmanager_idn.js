


function getPrincipalFromDomain(aDomain) {
  return Cc["@mozilla.org/scriptsecuritymanager;1"]
           .getService(Ci.nsIScriptSecurityManager)
           .getNoAppCodebasePrincipal(NetUtil.newURI("http://" + aDomain));
}

function run_test() {
  let profile = do_get_profile();
  let pm = Services.permissions;
  let perm = 'test-idn';

  
  
  
  let mainDomainPrincipal = getPrincipalFromDomain("fôû.com");
  let subDomainPrincipal = getPrincipalFromDomain("fôô.bàr.com");
  let tldPrincipal = getPrincipalFromDomain("fôû.bàr.côm");

  
  pm.addFromPrincipal(mainDomainPrincipal, perm, pm.ALLOW_ACTION, 0, 0);
  pm.addFromPrincipal(subDomainPrincipal, perm, pm.ALLOW_ACTION, 0, 0);
  pm.addFromPrincipal(tldPrincipal, perm, pm.ALLOW_ACTION, 0, 0);

  
  do_check_eq(pm.testPermissionFromPrincipal(mainDomainPrincipal, perm), pm.ALLOW_ACTION);
  do_check_eq(pm.testPermissionFromPrincipal(subDomainPrincipal, perm), pm.ALLOW_ACTION);
  do_check_eq(pm.testPermissionFromPrincipal(tldPrincipal, perm), pm.ALLOW_ACTION);

  
  let punyMainDomainPrincipal = getPrincipalFromDomain('xn--f-xgav.com');
  let punySubDomainPrincipal = getPrincipalFromDomain('xn--f-xgaa.xn--br-jia.com');
  let punyTldPrincipal = getPrincipalFromDomain('xn--f-xgav.xn--br-jia.xn--cm-8ja');

  
  do_check_eq(pm.testPermissionFromPrincipal(punyMainDomainPrincipal, perm), pm.ALLOW_ACTION);
  do_check_eq(pm.testPermissionFromPrincipal(punySubDomainPrincipal, perm), pm.ALLOW_ACTION);
  do_check_eq(pm.testPermissionFromPrincipal(punyTldPrincipal, perm), pm.ALLOW_ACTION);

  
  
  let witnessPrincipal = getPrincipalFromDomain("foo.com");
  do_check_eq(pm.testPermissionFromPrincipal(witnessPrincipal, perm), pm.UNKNOWN_ACTION);
  witnessPrincipal = getPrincipalFromDomain("foo.bar.com");
  do_check_eq(pm.testPermissionFromPrincipal(witnessPrincipal, perm), pm.UNKNOWN_ACTION);
}