






const profileDir = gProfD.clone();
profileDir.append("extensions");

function run_test() {
  do_test_pending();
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "2", "1.9.2");

  var dest = writeInstallRDFForExtension({
    id: "addon1@tests.mozilla.org",
    version: "1.0",
    name: "Test",
    targetApplications: [{
      id: "xpcshell@tests.mozilla.org",
      minVersion: "1",
      maxVersion: "1"
    }]
  }, profileDir);
  
  
  setExtensionModifiedTime(dest, dest.lastModifiedTime - 5000);

  startupManager();

  AddonManager.getAddonByID("addon1@tests.mozilla.org", callback_soon(function(a) {
    do_check_neq(a, null);
    do_check_eq(a.version, "1.0");
    do_check_false(a.userDisabled);
    do_check_true(a.appDisabled);
    do_check_false(a.isActive);
    do_check_false(isExtensionInAddonsList(profileDir, a.id));

    writeInstallRDFForExtension({
      id: "addon1@tests.mozilla.org",
      version: "2.0",
      name: "Test",
      targetApplications: [{
        id: "xpcshell@tests.mozilla.org",
        minVersion: "1",
        maxVersion: "2"
      }]
    }, profileDir);

    restartManager();

    AddonManager.getAddonByID("addon1@tests.mozilla.org", function(a) {
      do_check_neq(a, null);
      do_check_eq(a.version, "2.0");
      do_check_false(a.userDisabled);
      do_check_false(a.appDisabled);
      do_check_true(a.isActive);
      do_check_true(isExtensionInAddonsList(profileDir, a.id));

      do_execute_soon(do_test_finished);
    });
  }));
}
