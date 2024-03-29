





Services.prefs.setBoolPref(PREF_EM_CHECK_UPDATE_SECURITY, false);
Services.prefs.setBoolPref(PREF_EM_STRICT_COMPATIBILITY, false);

var ADDONS = [
  "test_bug470377_1",
  "test_bug470377_2",
  "test_bug470377_3",
  "test_bug470377_4",
  "test_bug470377_5",
];

Components.utils.import("resource://testing-common/httpd.js");
var server;

function run_test() {
  do_test_pending();
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "2", "2");

  server = new HttpServer();
  server.registerDirectory("/", do_get_file("data/test_bug470377"));
  server.start(-1);

  startupManager();

  installAllFiles([do_get_addon(a) for each (a in ADDONS)], function() {
    restartManager();

    AddonManager.getAddonsByIDs(["bug470377_1@tests.mozilla.org",
                                 "bug470377_2@tests.mozilla.org",
                                 "bug470377_3@tests.mozilla.org",
                                 "bug470377_4@tests.mozilla.org",
                                 "bug470377_5@tests.mozilla.org"],
                                 function([a1, a2, a3, a4, a5]) {
      do_check_eq(a1, null);
      do_check_neq(a2, null);
      do_check_neq(a3, null);
      do_check_neq(a4, null);
      do_check_neq(a5, null);

      server.stop(do_test_finished);
    });
  }, true);
}
