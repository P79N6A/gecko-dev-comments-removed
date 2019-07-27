






const PREF_XPI_WHITELIST_PERMISSIONS  = "xpinstall.whitelist.add";
const PREF_XPI_BLACKLIST_PERMISSIONS  = "xpinstall.blacklist.add";

function do_check_permission_prefs(preferences) {
  
  for (let pref of preferences) {
    try {
      do_check_eq(Services.prefs.getCharPref(pref), "");
    }
    catch (e) {
      
    }
  }
}

function clear_imported_preferences_cache() {
  let scope = Components.utils.import("resource://gre/modules/PermissionsUtils.jsm", {});
  scope.gImportedPrefBranches.clear();
}

function run_test() {
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "1.9");

  
  Services.prefs.setCharPref("xpinstall.whitelist.add.EMPTY", "");
  Services.prefs.setCharPref("xpinstall.whitelist.add.TEST", "http://whitelist.example.com");
  Services.prefs.setCharPref("xpinstall.blacklist.add.EMPTY", "");
  Services.prefs.setCharPref("xpinstall.blacklist.add.TEST", "http://blacklist.example.com");

  
  var whitelistPreferences = Services.prefs.getChildList(PREF_XPI_WHITELIST_PERMISSIONS, {});
  var blacklistPreferences = Services.prefs.getChildList(PREF_XPI_BLACKLIST_PERMISSIONS, {});
  var preferences = whitelistPreferences.concat(blacklistPreferences);

  startupManager();

  
  
  let url = Services.io.newURI("http://example.com/file.xpi", null, null);
  AddonManager.isInstallAllowed("application/x-xpinstall", url);
  do_check_permission_prefs(preferences);


  
  

  
  clear_imported_preferences_cache();
  Services.prefs.setCharPref("xpinstall.whitelist.add.TEST2", "https://whitelist2.example.com");
  Services.obs.notifyObservers(null, "flush-pending-permissions", "install");
  do_check_permission_prefs(preferences);

  
  clear_imported_preferences_cache();
  Services.prefs.setCharPref("xpinstall.whitelist.add.TEST3", "https://whitelist3.example.com");
  Services.obs.notifyObservers(null, "flush-pending-permissions", "");
  do_check_permission_prefs(preferences);

  
  clear_imported_preferences_cache();
  Services.prefs.setCharPref("xpinstall.whitelist.add.TEST4", "https://whitelist4.example.com");
  Services.obs.notifyObservers(null, "flush-pending-permissions", "lolcats");
  do_check_eq(Services.prefs.getCharPref("xpinstall.whitelist.add.TEST4"), "https://whitelist4.example.com");
}
