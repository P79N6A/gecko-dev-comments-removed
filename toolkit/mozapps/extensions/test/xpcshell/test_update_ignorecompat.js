







const PREF_GETADDONS_BYIDS = "extensions.getAddons.get.url";
const PREF_GETADDONS_CACHE_ENABLED = "extensions.getAddons.cache.enabled";


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

  run_test_1();
}



function run_test_1() {
  writeInstallRDFForExtension({
    id: "addon9@tests.mozilla.org",
    version: "1.0",
    updateURL: "http://localhost:4444/data/test_update.rdf",
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
      end_test();
    }
  });

  Services.prefs.setCharPref(PREF_GETADDONS_BYIDS, "http://localhost:4444/data/test_update.xml");
  Services.prefs.setBoolPref(PREF_GETADDONS_CACHE_ENABLED, true);
  
  gInternalManager.notify(null);
}

