






































gPrefs.setBoolPref("extensions.checkUpdateSecurity", false);


const checkListener = {
  
  onUpdateStarted: function onUpdateStarted() {
  },

  
  onUpdateEnded: function onUpdateEnded() {
    var item = gEM.getItemForID(ADDON.id);
    do_check_eq(item.version, 0.1);
    do_check_eq(item.targetAppID, "xpcshell@tests.mozilla.org");
    do_check_eq(item.minAppVersion, 1);
    do_check_eq(item.maxAppVersion, 1);

    testserver.stop(do_test_finished);
  },

  
  onAddonUpdateStarted: function onAddonUpdateStarted(aAddon) {
  },

  
  onAddonUpdateEnded: function onAddonUpdateEnded(aAddon, aStatus) {
  }
}


do_load_httpd_js();
var testserver;

var ADDON = {
  id: "bug299716-2@tests.mozilla.org",
  addon: "test_bug299716_2"
};

function run_test() {
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "1.9");

  const dataDir = do_get_file("data");
  const addonsDir = do_get_addon(ADDON.addon).parent;

  
  testserver = new nsHttpServer();
  testserver.registerDirectory("/addons/", addonsDir);
  testserver.registerDirectory("/data/", dataDir);
  testserver.start(4444);

  startupEM();

  
  gEM.installItemFromFile(do_get_addon(ADDON.addon),
                          NS_INSTALL_LOCATION_APPPROFILE);
  restartEM();

  var item = gEM.getItemForID(ADDON.id);
  do_check_eq(item.version, 0.1);
  do_check_eq(item.targetAppID, "xpcshell@tests.mozilla.org");
  do_check_eq(item.minAppVersion, 1);
  do_check_eq(item.maxAppVersion, 1);

  do_test_pending();

  gEM.update([item], 1,
             Components.interfaces.nsIExtensionManager.UPDATE_SYNC_COMPATIBILITY,
             checkListener);
}
