





const profileDir = gProfD.clone();
profileDir.append("extensions");

function run_test() {
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "1");

  startupManager();

  if (TEST_UNPACKED)
    run_test_unpacked();
  else
    run_test_packed();
}



function run_test_packed() {
  do_test_pending();

  prepare_test({
    "corrupt@tests.mozilla.org": [
      ["onInstalling", false],
      ["onInstalled", false]
    ]
  }, [
    "onNewInstall",
    "onInstallStarted",
    "onInstallEnded"
  ]);

  installAllFiles([do_get_file("data/corruptfile.xpi")], function() {
    ensure_test_completed();

    AddonManager.getAddonByID("corrupt@tests.mozilla.org", function(addon) {
      do_check_neq(addon, null);

      do_test_finished();
    });
  });
}



function run_test_unpacked() {
  do_test_pending();

  prepare_test({
    "corrupt@tests.mozilla.org": [
      ["onInstalling", false],
      "onOperationCancelled"
    ]
  }, [
    "onNewInstall",
    "onInstallStarted",
    "onInstallFailed"
  ]);

  installAllFiles([do_get_file("data/corruptfile.xpi")], function() {
    ensure_test_completed();

    
    var addonDir = profileDir.clone();
    addonDir.append("corrupt@tests.mozilla.org");
    pathShouldntExist(addonDir);

    
    var stageDir = profileDir.clone();
    stageDir.append("staged");
    pathShouldntExist(stageDir);

    AddonManager.getAddonByID("corrupt@tests.mozilla.org", function(addon) {
      do_check_eq(addon, null);

      do_test_finished();
    });
  });
}
