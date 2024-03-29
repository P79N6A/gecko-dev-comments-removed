









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
  Services.prefs.setCharPref(PREF_ROOT + "whitelist.add.TEST", "http://whitelist.example.com");
  Services.prefs.setCharPref(PREF_ROOT + "whitelist.add.TEST2", "https://whitelist2-1.example.com,http://whitelist2-2.example.com:8080,about:home");
  Services.prefs.setCharPref(PREF_ROOT + "whitelist.add.TEST3", "whitelist3-1.example.com,about:config"); 
  Services.prefs.setCharPref(PREF_ROOT + "blacklist.add.EMPTY", "");
  Services.prefs.setCharPref(PREF_ROOT + "blacklist.add.TEST", "http://blacklist.example.com,");
  Services.prefs.setCharPref(PREF_ROOT + "blacklist.add.TEST2", ",https://blacklist2-1.example.com,http://blacklist2-2.example.com:8080,about:mozilla");
  Services.prefs.setCharPref(PREF_ROOT + "blacklist.add.TEST3", "blacklist3-1.example.com,about:preferences"); 

  
  let whitelisted = ["http://whitelist.example.com",
                     "https://whitelist2-1.example.com",
                     "http://whitelist2-2.example.com:8080",
                     "http://whitelist3-1.example.com",
                     "https://whitelist3-1.example.com",
                     "about:config",
                     "about:home"];
  let blacklisted = ["http://blacklist.example.com",
                     "https://blacklist2-1.example.com",
                     "http://blacklist2-2.example.com:8080",
                     "http://blacklist3-1.example.com",
                     "https://blacklist3-1.example.com",
                     "about:preferences",
                     "about:mozilla"];
  let untouched = ["https://whitelist.example.com",
                   "https://blacklist.example.com",
                   "http://whitelist2-1.example.com",
                   "http://blacklist2-1.example.com",
                   "https://whitelist2-2.example.com:8080",
                   "https://blacklist2-2.example.com:8080"];
  let unknown = whitelisted.concat(blacklisted).concat(untouched);
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
  for (let url of untouched) {
    let uri = Services.io.newURI(url, null, null);
    do_check_eq(Services.perms.testPermission(uri, TEST_PERM), Services.perms.UNKNOWN_ACTION);
  }
}
