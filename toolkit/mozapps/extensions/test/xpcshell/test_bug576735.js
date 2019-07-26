






var addon1 = {
  id: "addon1@tests.mozilla.org",
  version: "1.0",
  name: "Test 1",
  targetApplications: [{
    id: "xpcshell@tests.mozilla.org",
    minVersion: "1",
    maxVersion: "1"
  }]
};

var addon2 = {
  id: "addon2@tests.mozilla.org",
  version: "2.0",
  name: "Test 2",
  targetApplications: [{
    id: "toolkit@mozilla.org",
    minVersion: "1",
    maxVersion: "1"
  }]
};

const profileDir = gProfD.clone();
profileDir.append("extensions");

function run_test() {
  do_test_pending();
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "2");

  writeInstallRDFForExtension(addon1, profileDir);

  startupManager();

  AddonManager.getAddonByID("addon1@tests.mozilla.org", function(a1) {
    a1.uninstall();

    shutdownManager();

    var dest = profileDir.clone();
    dest.append(do_get_expected_addon_name("addon1@tests.mozilla.org"));
    dest.remove(true);

    writeInstallRDFForExtension(addon2, profileDir);

    startupManager();

    AddonManager.getAddonsByIDs(["addon1@tests.mozilla.org",
                                 "addon2@tests.mozilla.org"],
                                function([a1, a2]) {
      
      do_check_eq(a1, null);

      
      do_check_neq(a2, null);

      do_test_finished();
    });
  });
}
