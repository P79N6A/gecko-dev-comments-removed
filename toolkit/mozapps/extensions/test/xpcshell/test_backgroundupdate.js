






Services.prefs.setBoolPref(PREF_EM_CHECK_UPDATE_SECURITY, false);

Components.utils.import("resource://testing-common/httpd.js");
var testserver = new HttpServer();
testserver.start(-1);
gPort = testserver.identity.primaryPort;
const profileDir = gProfD.clone();
profileDir.append("extensions");


mapFile("/data/test_backgroundupdate.rdf", testserver);

function run_test() {
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "1.9.2");

  testserver.registerDirectory("/addons/", do_get_file("addons"));

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

      do_execute_soon(run_test_2);
    }, "addons-background-update-complete", false);

    
    gInternalManager.notify(null);
  });
}



function run_test_2() {
  writeInstallRDFForExtension({
    id: "addon1@tests.mozilla.org",
    version: "1.0",
    updateURL: "http://localhost:" + gPort + "/data/test_backgroundupdate.rdf",
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
    updateURL: "http://localhost:" + gPort + "/data/test_backgroundupdate.rdf",
    targetApplications: [{
      id: "xpcshell@tests.mozilla.org",
      minVersion: "1",
      maxVersion: "1"
    }],
    name: "Test Addon 2",
  }, profileDir);

  writeInstallRDFForExtension({
    id: "addon3@tests.mozilla.org",
    version: "1.0",
    targetApplications: [{
      id: "xpcshell@tests.mozilla.org",
      minVersion: "1",
      maxVersion: "1"
    }],
    name: "Test Addon 3",
  }, profileDir);

  
  Services.prefs.setCharPref("extensions.update.background.url",
                             "http://localhost:" + gPort +"/data/test_backgroundupdate.rdf");
  restartManager();

  
  Services.prefs.setCharPref("extensions.hotfix.id", "hotfix@tests.mozilla.org");
  Services.prefs.setCharPref("extensions.hotfix.url", "http://localhost:" + gPort + "/missing.rdf");

  let installCount = 0;
  let completeCount = 0;
  let sawCompleteNotification = false;

  Services.obs.addObserver(function() {
    Services.obs.removeObserver(arguments.callee, "addons-background-update-complete");

    do_check_eq(installCount, 3);
    sawCompleteNotification = true;
  }, "addons-background-update-complete", false);

  AddonManager.addInstallListener({
    onNewInstall: function(aInstall) {
      installCount++;
    },

    onDownloadFailed: function(aInstall) {
      completeCount++;
      if (completeCount == 3) {
        do_check_true(sawCompleteNotification);
        end_test();
      }
    }
  });

  
  gInternalManager.notify(null);
}
