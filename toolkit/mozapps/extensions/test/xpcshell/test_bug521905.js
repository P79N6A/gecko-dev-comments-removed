




const ADDON = "test_bug521905";
const ID = "bug521905@tests.mozilla.org";


Services.prefs.setBoolPref("extensions.checkUpdateSecurity", false);

function run_test() {
  
  
  if (isNightlyChannel()) {
    return;
  }

  do_test_pending();
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "2.0pre", "2");

  startupManager();
  AddonManager.checkCompatibility = false;

  installAllFiles([do_get_addon(ADDON)], function() {
    restartManager();

    AddonManager.getAddonByID(ID, function(addon) {
      do_check_neq(addon, null);
      do_check_true(addon.isActive);

      run_test_1();
    });
  });
}

function run_test_1() {
  Services.prefs.setBoolPref("extensions.checkCompatibility.2.0pre", true);

  restartManager();
  AddonManager.getAddonByID(ID, function(addon) {
    do_check_neq(addon, null);
    do_check_false(addon.isActive);

    run_test_2();
  });
}

function run_test_2() {
  Services.prefs.setBoolPref("extensions.checkCompatibility.2.0p", false);

  restartManager();
  AddonManager.getAddonByID(ID, function(addon) {
    do_check_neq(addon, null);
    do_check_false(addon.isActive);

    do_test_finished();
  });
}
