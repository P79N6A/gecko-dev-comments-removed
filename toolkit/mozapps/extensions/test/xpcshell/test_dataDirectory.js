





Services.prefs.setBoolPref("extensions.checkUpdateSecurity", false);

var ADDON = {
  id: "datadirectory1@tests.mozilla.org",
  addon: "test_data_directory"
};

var expectedDir = gProfD.clone();
expectedDir.append("extension-data");
expectedDir.append(ADDON.id);

function run_test() {
    do_test_pending();
    do_check_false(expectedDir.exists());

    createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "2", "1.9");
    startupManager();

    installAllFiles([do_get_addon(ADDON.addon)], function() {
        restartManager();

        AddonManager.getAddonByID(ADDON.id, function(item) {
            item.getDataDirectory(promise_callback);
        });
    });
}

function promise_callback() {
    do_check_eq(arguments.length, 2);
    var expectedDir = gProfD.clone();
    expectedDir.append("extension-data");
    expectedDir.append(ADDON.id);

    do_check_eq(arguments[0], expectedDir.path);
    do_check_true(expectedDir.exists());
    do_check_true(expectedDir.isDirectory());

    do_check_eq(arguments[1], null);

    
    expectedDir.parent.remove(true);

    do_test_finished();
}
