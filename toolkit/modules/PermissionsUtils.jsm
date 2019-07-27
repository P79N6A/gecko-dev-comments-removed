



this.EXPORTED_SYMBOLS = ["PermissionsUtils"];

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/BrowserUtils.jsm")


let gImportedPrefBranches = new Set();

function importPrefBranch(aPrefBranch, aPermission, aAction) {
  let list = Services.prefs.getChildList(aPrefBranch, {});

  for (let pref of list) {
    let origins = "";
    try {
      origins = Services.prefs.getCharPref(pref);
    } catch (e) {}

    if (!origins)
      continue;

    origins = origins.split(",");

    for (let origin of origins) {
      let principals = [];
      try {
        principals = [ BrowserUtils.principalFromOrigin(origin) ];
      } catch (e) {
        
        
        
        try {
          let httpURI = Services.io.newURI("http://" + origin, null, null);
          let httpsURI = Services.io.newURI("https://" + origin, null, null);

          principals = [
            Services.scriptSecurityManager.getNoAppCodebasePrincipal(httpURI),
            Services.scriptSecurityManager.getNoAppCodebasePrincipal(httpsURI)
          ];
        } catch (e2) {}
      }

      for (let principal of principals) {
        try {
          Services.perms.addFromPrincipal(principal, aPermission, aAction);
        } catch (e) {}
      }
    }

    Services.prefs.setCharPref(pref, "");
  }
}


this.PermissionsUtils = {
  






















  importFromPrefs: function(aPrefBranch, aPermission) {
    if (!aPrefBranch.endsWith("."))
      aPrefBranch += ".";

    
    if (gImportedPrefBranches.has(aPrefBranch))
     return;

    importPrefBranch(aPrefBranch + "whitelist.add", aPermission,
                     Services.perms.ALLOW_ACTION);
    importPrefBranch(aPrefBranch + "blacklist.add", aPermission,
                     Services.perms.DENY_ACTION);

    gImportedPrefBranches.add(aPrefBranch);
  }
};
