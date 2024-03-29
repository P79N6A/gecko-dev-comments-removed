


function matches_always(perm, uris) {
  uris.forEach((uri) => {
    do_check_true(perm.matchesURI(uri, true), "perm: " + perm.principal.origin + ", URI: " + uri.spec);
    do_check_true(perm.matchesURI(uri, false), "perm: " + perm.principal.origin + ", URI: " + uri.spec);
  });
}

function matches_weak(perm, uris) {
  uris.forEach((uri) => {
    do_check_false(perm.matchesURI(uri, true), "perm: " + perm.principal.origin + ", URI: " + uri.spec);
    do_check_true(perm.matchesURI(uri, false), "perm: " + perm.principal.origin + ", URI: " + uri.spec);
  });
}

function matches_never(perm, uris) {
  uris.forEach((uri) => {
    do_check_false(perm.matchesURI(uri, true), "perm: " + perm.principal.origin + ", URI: " + uri.spec);
    do_check_false(perm.matchesURI(uri, false), "perm: " + perm.principal.origin + ", URI: " + uri.spec);
  });
}

function mk_permission(uri, isAppPermission = false) {
  let pm = Cc["@mozilla.org/permissionmanager;1"].
        getService(Ci.nsIPermissionManager);

  let secMan = Cc["@mozilla.org/scriptsecuritymanager;1"]
        .getService(Ci.nsIScriptSecurityManager);

  
  let principal = isAppPermission ?
        secMan.getAppCodebasePrincipal(uri, 1000, false) :
        secMan.getNoAppCodebasePrincipal(uri);

  pm.addFromPrincipal(principal, "test/matchesuri", pm.ALLOW_ACTION);
  let permission = pm.getPermissionObject(principal, "test/matchesuri", true);

  return permission;
}

function run_test() {
  
  let pm = Cc["@mozilla.org/permissionmanager;1"].
        getService(Ci.nsIPermissionManager);

  let secMan = Cc["@mozilla.org/scriptsecuritymanager;1"]
        .getService(Ci.nsIScriptSecurityManager);

  let fileprefix = "file:///";
  if (Services.appinfo.OS == "WINNT") {
    
    fileprefix += "c:/";
  }

  
  let uri0 = NetUtil.newURI("http://google.com:9091/just/a/path", null, null);
  let uri1 = NetUtil.newURI("http://hangouts.google.com:9091/some/path", null, null);
  let uri2 = NetUtil.newURI("http://google.com:9091/", null, null);
  let uri3 = NetUtil.newURI("http://google.org:9091/", null, null);
  let uri4 = NetUtil.newURI("http://deeper.hangouts.google.com:9091/", null, null);
  let uri5 = NetUtil.newURI("https://google.com/just/a/path", null, null);
  let uri6 = NetUtil.newURI("https://hangouts.google.com", null, null);
  let uri7 = NetUtil.newURI("https://google.com/", null, null);

  let fileuri1 = NetUtil.newURI(fileprefix + "a/file/path", null, null);
  let fileuri2 = NetUtil.newURI(fileprefix + "a/file/path/deeper", null, null);
  let fileuri3 = NetUtil.newURI(fileprefix + "a/file/otherpath", null, null);

  {
    let perm = mk_permission(uri0);
    matches_always(perm, [uri0, uri2]);
    matches_weak(perm, [uri1, uri4]);
    matches_never(perm, [uri3, uri5, uri6, uri7, fileuri1, fileuri2, fileuri3]);
  }

  {
    let perm = mk_permission(uri1);
    matches_always(perm, [uri1]);
    matches_weak(perm, [uri4]);
    matches_never(perm, [uri0, uri2, uri3, uri5, uri6, uri7, fileuri1, fileuri2, fileuri3]);
  }

  {
    let perm = mk_permission(uri2);
    matches_always(perm, [uri0, uri2]);
    matches_weak(perm, [uri1, uri4]);
    matches_never(perm, [uri3, uri5, uri6, uri7, fileuri1, fileuri2, fileuri3]);
  }

  {
    let perm = mk_permission(uri3);
    matches_always(perm, [uri3]);
    matches_weak(perm, []);
    matches_never(perm, [uri1, uri2, uri4, uri5, uri6, uri7, fileuri1, fileuri2, fileuri3]);
  }

  {
    let perm = mk_permission(uri4);
    matches_always(perm, [uri4]);
    matches_weak(perm, []);
    matches_never(perm, [uri1, uri2, uri3, uri5, uri6, uri7, fileuri1, fileuri2, fileuri3]);
  }

  {
    let perm = mk_permission(uri5);
    matches_always(perm, [uri5, uri7]);
    matches_weak(perm, [uri6]);
    matches_never(perm, [uri0, uri1, uri2, uri3, uri4, fileuri1, fileuri2, fileuri3]);
  }

  {
    let perm = mk_permission(uri6);
    matches_always(perm, [uri6]);
    matches_weak(perm, []);
    matches_never(perm, [uri0, uri1, uri2, uri3, uri4, uri5, uri7, fileuri1, fileuri2, fileuri3]);
  }

  {
    let perm = mk_permission(uri7);
    matches_always(perm, [uri5, uri7]);
    matches_weak(perm, [uri6]);
    matches_never(perm, [uri0, uri1, uri2, uri3, uri4, fileuri1, fileuri2, fileuri3]);
  }

  {
    let perm = mk_permission(fileuri1);
    matches_always(perm, [fileuri1]);
    matches_weak(perm, []);
    matches_never(perm, [uri0, uri1, uri2, uri3, uri4, uri5, uri6, uri7, fileuri2, fileuri3]);
  }

  {
    let perm = mk_permission(fileuri2);
    matches_always(perm, [fileuri2]);
    matches_weak(perm, []);
    matches_never(perm, [uri0, uri1, uri2, uri3, uri4, uri5, uri6, uri7, fileuri1, fileuri3]);
  }

  {
    let perm = mk_permission(fileuri3);
    matches_always(perm, [fileuri3]);
    matches_weak(perm, []);
    matches_never(perm, [uri0, uri1, uri2, uri3, uri4, uri5, uri6, uri7, fileuri1, fileuri2]);
  }

  
  pm.removeAll();
}
