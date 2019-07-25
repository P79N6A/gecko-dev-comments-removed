






Services.prefs.setBoolPref(PREF_EM_CHECK_UPDATE_SECURITY, false);

do_load_httpd_js();
var testserver;
const profileDir = gProfD.clone();
profileDir.append("extensions");

function run_test() {
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "1.9.2");

  
  testserver = new nsHttpServer();
  testserver.registerDirectory("/data/", do_get_file("data"));
  testserver.registerDirectory("/addons/", do_get_file("addons"));
  testserver.start(4444);

  startupManager();

  do_test_pending();
  run_test_1();
}

function end_test() {
  testserver.stop(do_test_finished);
}



function run_test_1() {
  AddonManager.getAddonsByTypes(["extension", "theme", "locale"], function(aAddons) {
    do_check_eq(aAddons.length, 0);

    Services.obs.addObserver(function() {
      Services.obs.removeObserver(arguments.callee, "addons-background-update-complete");

      run_test_2();
    }, "addons-background-update-complete", false);

    AddonManagerPrivate.backgroundUpdateCheck();
  });
}



function run_test_2() {
  writeInstallRDFForExtension({
    id: "addon1@tests.mozilla.org",
    version: "1.0",
    updateURL: "http://localhost:4444/data/test_backgroundupdate.rdf",
    targetApplications: [{
      id: "xpcshell@tests.mozilla.org",
      minVersion: "1",
      maxVersion: "1"
    }],
    name: "Test Addon 1",
  }, profileDir);

  writeInstallRDFForExtension({
    id: "addon2@tests.mozilla.org",
    version: "1.0",
    updateURL: "http://localhost:4444/data/test_backgroundupdate.rdf",
    targetApplications: [{
      id: "xpcshell@tests.mozilla.org",
      minVersion: "1",
      maxVersion: "1"
    }],
    name: "Test Addon 2",
  }, profileDir);

  restartManager();

  let installCount = 0;
  let completeCount = 0;
  let sawCompleteNotification = false;

  Services.obs.addObserver(function() {
    Services.obs.removeObserver(arguments.callee, "addons-background-update-complete");

    do_check_eq(installCount, 2);
    sawCompleteNotification = true;
  }, "addons-background-update-complete", false);

  AddonManager.addInstallListener({
    onNewInstall: function(aInstall) {
      installCount++;
    },

    onDownloadFailed: function(aInstall) {
      completeCount++;
      if (completeCount == 2) {
        do_check_true(sawCompleteNotification);
        end_test();
      }
    }
  });

  AddonManagerPrivate.backgroundUpdateCheck();
}
