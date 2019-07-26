




var ADDONS = [
  "test_bootstrap2_1", 
  "test_bootstrap1_4", 
  "test_jetpack"       
];

var IDS = [
  "bootstrap1@tests.mozilla.org",
  "bootstrap2@tests.mozilla.org",
  "jetpack@tests.mozilla.org"
];

function run_test() {
  do_test_pending();
  
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "2", "2");

  startupManager();
  AddonManager.checkCompatibility = false;

  installAllFiles(ADDONS.map(do_get_addon), function () {
    restartManager();

    AddonManager.getAddonsByIDs(IDS, function([a1, a2, a3]) {
      do_check_eq(a1.isDebuggable, false);
      do_check_eq(a2.isDebuggable, true);
      do_check_eq(a3.isDebuggable, true);
      do_test_finished();
    });
  }, true);
}
