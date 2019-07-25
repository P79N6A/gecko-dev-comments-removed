





Services.prefs.setBoolPref("extensions.checkUpdateSecurity", false);


Components.utils.import("resource://testing-common/httpd.js");
var testserver;

var ADDON = {
  id: "bug299716-2@tests.mozilla.org",
  addon: "test_bug299716_2"
};

function run_test() {
  do_test_pending();

  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "2", "1.9");

  const dataDir = do_get_file("data");
  const addonsDir = do_get_addon(ADDON.addon).parent;

  
  testserver = new HttpServer();
  testserver.registerDirectory("/addons/", addonsDir);
  testserver.registerDirectory("/data/", dataDir);
  testserver.start(4444);

  startupManager();

  installAllFiles([do_get_addon(ADDON.addon)], function() {
    restartManager();

    AddonManager.getAddonByID(ADDON.id, function(item) {
      do_check_eq(item.version, 0.1);
      do_check_false(item.isCompatible);

      item.findUpdates({
        onUpdateFinished: function(addon) {
          do_check_false(item.isCompatible);

          testserver.stop(do_test_finished);
        }
      }, AddonManager.UPDATE_WHEN_USER_REQUESTED);
    });
  });
}
