





































const ADDON = "test_bug521905";
const ID = "bug521905@tests.mozilla.org";


Services.prefs.setBoolPref("extensions.checkUpdateSecurity", false);

Services.prefs.setBoolPref("extensions.checkCompatibility.2.0pre", false);

function run_test() {
  var channel = "default";
  try {
    channel = Services.prefs.getCharPref("app.update.channel");
  }
  catch (e) { }

  
  
  if (channel != "aurora" &&
      channel != "beta" &&
      channel != "release") {
    return;
  }

  do_test_pending();
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "2.0pre", "2");

  startupManager();
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
