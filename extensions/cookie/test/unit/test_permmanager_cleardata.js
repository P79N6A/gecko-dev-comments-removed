


let pm;


function createPrincipal(aOrigin, aAppId, aBrowserElement)
{
  return Services.scriptSecurityManager.getAppCodebasePrincipal(NetUtil.newURI(aOrigin), aAppId, aBrowserElement);
}


function getSubject(aAppId, aBrowserOnly)
{
  return {
    appId: aAppId,
    browserOnly: aBrowserOnly,
    QueryInterface: XPCOMUtils.generateQI([Ci.mozIApplicationClearPrivateDataParams])
  };
}




function test(aEntries, aSubject, aResults)
{
  let principals = [];

  for (entry of aEntries) {
    principals.push(createPrincipal(entry.origin, entry.appId, entry.browserElement));
  }

  for (principal of principals) {
    do_check_eq(pm.testPermissionFromPrincipal(principal, "test/webapps-clear"), pm.UNKNOWN_ACTION);
    pm.addFromPrincipal(principal, "test/webapps-clear", pm.ALLOW_ACTION, pm.EXPIRE_NEVER, 0);
    do_check_eq(pm.testPermissionFromPrincipal(principal, "test/webapps-clear"), pm.ALLOW_ACTION);
  }

  Services.obs.notifyObservers(aSubject, 'webapps-clear-data', null);

  var length = aEntries.length;
  for (let i=0; i<length; ++i) {
    do_check_eq(pm.testPermissionFromPrincipal(principals[i], 'test/webapps-clear'), aResults[i]);

    
    if (aResults[i] == pm.ALLOW_ACTION) {
      pm.removeFromPrincipal(principals[i], 'test/webapps-clear');
    }
  }
}

function run_test()
{
  do_get_profile();

  pm = Cc["@mozilla.org/permissionmanager;1"]
         .getService(Ci.nsIPermissionManager);

  let entries = [
    { origin: 'http://example.com', appId: 1, browserElement: false },
    { origin: 'http://example.com', appId: 1, browserElement: true },
    { origin: 'http://example.com', appId: Ci.nsIScriptSecurityManager.NO_APPID, browserElement: false },
    { origin: 'http://example.com', appId: 2, browserElement: false },
  ];

  
  test(entries, getSubject(1, false), [ pm.UNKNOWN_ACTION, pm.UNKNOWN_ACTION, pm.ALLOW_ACTION, pm.ALLOW_ACTION ]);

  
  
  test(entries, getSubject(1, true), [ pm.ALLOW_ACTION, pm.UNKNOWN_ACTION, pm.ALLOW_ACTION, pm.ALLOW_ACTION ]);
}
