

var hosts = [
  
  ["http://mozilla.org", "cookie", 1],
  ["http://mozilla.org", "image", 2],
  ["http://mozilla.org", "popup", 3],
  ["http://mozilla.com", "cookie", 1],
  ["http://www.mozilla.com", "cookie", 2],
  ["http://dev.mozilla.com", "cookie", 3]
];

var results = [
  
  
  ["http://localhost", "cookie", 0, 0],
  ["http://spreadfirefox.com", "cookie", 0, 0],
  
  ["http://mozilla.org", "cookie", 1, 1],
  ["http://mozilla.org", "image", 2, 2],
  ["http://mozilla.org", "popup", 3, 3],
  
  ["http://www.mozilla.org", "cookie", 1, 0],
  ["http://www.dev.mozilla.org", "cookie", 1, 0],
  
  ["http://mozilla.com", "cookie", 1, 1],
  ["http://www.mozilla.com", "cookie", 2, 2],
  ["http://dev.mozilla.com", "cookie", 3, 3],
  ["http://www.dev.mozilla.com", "cookie", 3, 0]
];

function run_test() {
  var pm = Components.classes["@mozilla.org/permissionmanager;1"]
                     .getService(Components.interfaces.nsIPermissionManager);

  var ioService = Components.classes["@mozilla.org/network/io-service;1"]
                            .getService(Components.interfaces.nsIIOService);

  var secMan = Components.classes["@mozilla.org/scriptsecuritymanager;1"]
                         .getService(Components.interfaces.nsIScriptSecurityManager);

  
  if (!pm)
    return;

  
  for (var i = 0; i < hosts.length; ++i) {
    let uri = ioService.newURI(hosts[i][0], null, null);
    let principal = secMan.getNoAppCodebasePrincipal(uri);

    pm.addFromPrincipal(principal, hosts[i][1], hosts[i][2]);
  }

  
  for (var i = 0; i < results.length; ++i) {
    let uri = ioService.newURI(results[i][0], null, null);
    let principal = secMan.getNoAppCodebasePrincipal(uri);

    do_check_eq(pm.testPermissionFromPrincipal(principal, results[i][1]), results[i][2]);
    do_check_eq(pm.testExactPermissionFromPrincipal(principal, results[i][1]), results[i][3]);
  }

  
  var j = 0;
  var perms = new Array();
  var enumerator = pm.enumerator;
  while (enumerator.hasMoreElements()) {
    perms[j] = enumerator.getNext().QueryInterface(Components.interfaces.nsIPermission);
    ++j;
  }
  do_check_eq(perms.length, hosts.length);

  
  for (var j = 0; j < perms.length; ++j) {
    pm.removePermission(perms[j]);
  }
  
  
  for (var i = 0; i < hosts.length; ++i) {
    for (var j = 0; j < perms.length; ++j) {
      if (perms[j].matchesURI(ioService.newURI(hosts[i][0], null, null), true) &&
          hosts[i][1] == perms[j].type &&
          hosts[i][2] == perms[j].capability) {
        perms.splice(j, 1);
        break;
      }
    }
  }
  do_check_eq(perms.length, 0);

  
  do_check_eq(pm.enumerator.hasMoreElements(), false);

  
  var utf8 = "b\u00FCcher.dolske.org"; 
  var aceref = "xn--bcher-kva.dolske.org";
  var uri = ioService.newURI("http://" + utf8, null, null);
  pm.add(uri, "utf8", 1);
  var enumerator = pm.enumerator;
  do_check_eq(enumerator.hasMoreElements(), true);
  var ace = enumerator.getNext().QueryInterface(Components.interfaces.nsIPermission);
  do_check_eq(ace.principal.URI.asciiHost, aceref);
  do_check_eq(enumerator.hasMoreElements(), false);

  
  pm.removeAll();
  do_check_eq(pm.enumerator.hasMoreElements(), false);
}
