









const PREF_ROOT = "testpermissions.";
const TEST_PERM = "test-permission";

Components.utils.import("resource://gre/modules/Services.jsm");
Components.utils.import("resource://gre/modules/PermissionsUtils.jsm");

function run_test() {
  test_importfromPrefs();
}


function test_importfromPrefs() {
  
  Services.prefs.setCharPref(PREF_ROOT + "whitelist.add.EMPTY", "");
  Services.prefs.setCharPref(PREF_ROOT + "whitelist.add.EMPTY2", ",");
  Services.prefs.setCharPref(PREF_ROOT + "whitelist.add.TEST", "whitelist.example.com");
  Services.prefs.setCharPref(PREF_ROOT + "whitelist.add.TEST2", "whitelist2-1.example.com,whitelist2-2.example.com,about:home");
  Services.prefs.setCharPref(PREF_ROOT + "blacklist.add.EMPTY", "");
  Services.prefs.setCharPref(PREF_ROOT + "blacklist.add.TEST", "blacklist.example.com,");
  Services.prefs.setCharPref(PREF_ROOT + "blacklist.add.TEST2", ",blacklist2-1.example.com,blacklist2-2.example.com,about:mozilla");

  
  let whitelisted = ["http://whitelist.example.com",
                     "http://whitelist2-1.example.com",
                     "http://whitelist2-2.example.com",
                     "about:home"];
  let blacklisted = ["http://blacklist.example.com",
                     "http://blacklist2-1.example.com",
                     "http://blacklist2-2.example.com",
                     "about:mozilla"];
  let unknown = whitelisted.concat(blacklisted);
  for (let url of unknown) {
    let uri = Services.io.newURI(url, null, null);
    do_check_eq(Services.perms.testPermission(uri, TEST_PERM), Services.perms.UNKNOWN_ACTION);
  }

  
  PermissionsUtils.importFromPrefs(PREF_ROOT, TEST_PERM);

  
  let preferences = Services.prefs.getChildList(PREF_ROOT, {});

  
  for (let pref of preferences) {
    do_check_eq(Services.prefs.getCharPref(pref), "");
  }

  
  for (let url of whitelisted) {
    let uri = Services.io.newURI(url, null, null);
    do_check_eq(Services.perms.testPermission(uri, TEST_PERM), Services.perms.ALLOW_ACTION);
  }
  for (let url of blacklisted) {
    let uri = Services.io.newURI(url, null, null);
    do_check_eq(Services.perms.testPermission(uri, TEST_PERM), Services.perms.DENY_ACTION);
  }
}
