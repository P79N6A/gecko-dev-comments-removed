






const PREF_GETADDONS_CACHE_ENABLED = "extensions.getAddons.cache.enabled";


Services.prefs.setBoolPref(PREF_EM_CHECK_UPDATE_SECURITY, false);

Components.utils.import("resource://testing-common/httpd.js");
var testserver = new HttpServer();
testserver.start(-1);
gPort = testserver.identity.primaryPort;
mapFile("/data/test_update.rdf", testserver);
mapFile("/data/test_update.xml", testserver);
testserver.registerDirectory("/addons/", do_get_file("addons"));

const profileDir = gProfD.clone();
profileDir.append("extensions");


function run_test() {
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "1.9.2");

  run_test_1();
}



function run_test_1() {
  writeInstallRDFForExtension({
    id: "addon9@tests.mozilla.org",
    version: "1.0",
    updateURL: "http://localhost:" + gPort + "/data/test_update.rdf",
    targetApplications: [{
      id: "xpcshell@tests.mozilla.org",
      minVersion: "0.1",
      maxVersion: "0.2"
    }],
    name: "Test Addon 9",
  }, profileDir);
  restartManager();

  AddonManager.addInstallListener({
    onNewInstall: function(aInstall) {
      if (aInstall.existingAddon.id != "addon9@tests.mozilla.org")
        do_throw("Saw unexpected onNewInstall for " + aInstall.existingAddon.id);
      do_check_eq(aInstall.version, "4.0");
    },
    onDownloadFailed: function(aInstall) {
      do_execute_soon(run_test_2);
    }
  });

  Services.prefs.setCharPref(PREF_GETADDONS_BYIDS_PERFORMANCE,
                             "http://localhost:" + gPort + "/data/test_update.xml");
  Services.prefs.setBoolPref(PREF_GETADDONS_CACHE_ENABLED, true);

  AddonManagerInternal.backgroundUpdateCheck();
}



function run_test_2() {
  writeInstallRDFForExtension({
    id: "addon11@tests.mozilla.org",
    version: "1.0",
    updateURL: "http://localhost:" + gPort + "/data/test_update.rdf",
    targetApplications: [{
      id: "xpcshell@tests.mozilla.org",
      minVersion: "0.1",
      maxVersion: "0.2"
    }],
    name: "Test Addon 11",
  }, profileDir);
  restartManager();

  AddonManager.getAddonByID("addon11@tests.mozilla.org", function(a11) {
    do_check_neq(a11, null);

    a11.findUpdates({
      onCompatibilityUpdateAvailable: function() {
        do_throw("Should have not have seen compatibility information");
      },

      onNoUpdateAvailable: function() {
        do_throw("Should have seen an available update");
      },

      onUpdateFinished: function() {
        end_test();
      }
    }, AddonManager.UPDATE_WHEN_USER_REQUESTED);
  });
}
