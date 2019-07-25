




createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "1.9.2");


Services.prefs.setIntPref("extensions.enabledScopes",
                          AddonManager.SCOPE_PROFILE + AddonManager.SCOPE_USER +
                          AddonManager.SCOPE_SYSTEM);

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
    id: "xpcshell@tests.mozilla.org",
    minVersion: "1",
    maxVersion: "2"
  }]
};

const addon1Dir = gProfD.clone();
addon1Dir.append("addon1");
writeInstallRDFToDir(addon1, addon1Dir);
const addon2Dir = gProfD.clone();
addon2Dir.append("addon2");
writeInstallRDFToDir(addon2, addon2Dir);

function run_test() {
  
  if (!("nsIWindowsRegKey" in AM_Ci))
    return;

  do_test_pending();

  run_test_1();
}


function run_test_1() {
  MockRegistry.setValue(AM_Ci.nsIWindowsRegKey.ROOT_KEY_LOCAL_MACHINE,
                        "SOFTWARE\\Mozilla\\XPCShell\\Extensions",
                        "addon1@tests.mozilla.org", addon1Dir.path);
  MockRegistry.setValue(AM_Ci.nsIWindowsRegKey.ROOT_KEY_CURRENT_USER,
                        "SOFTWARE\\Mozilla\\XPCShell\\Extensions",
                        "addon2@tests.mozilla.org", addon2Dir.path);

  startupManager();

  AddonManager.getAddonsByIDs(["addon1@tests.mozilla.org",
                               "addon2@tests.mozilla.org"], function([a1, a2]) {
    do_check_neq(a1, null);
    do_check_true(a1.isActive);
    do_check_false(hasFlag(a1.permissions, AddonManager.PERM_CAN_UNINSTALL));
    do_check_eq(a1.scope, AddonManager.SCOPE_SYSTEM);

    do_check_neq(a2, null);
    do_check_true(a2.isActive);
    do_check_false(hasFlag(a2.permissions, AddonManager.PERM_CAN_UNINSTALL));
    do_check_eq(a2.scope, AddonManager.SCOPE_USER);

    run_test_2();
  });
}


function run_test_2() {
  MockRegistry.setValue(AM_Ci.nsIWindowsRegKey.ROOT_KEY_LOCAL_MACHINE,
                        "SOFTWARE\\Mozilla\\XPCShell\\Extensions",
                        "addon1@tests.mozilla.org", null);
  MockRegistry.setValue(AM_Ci.nsIWindowsRegKey.ROOT_KEY_CURRENT_USER,
                        "SOFTWARE\\Mozilla\\XPCShell\\Extensions",
                        "addon2@tests.mozilla.org", null);

  restartManager();

  AddonManager.getAddonsByIDs(["addon1@tests.mozilla.org",
                               "addon2@tests.mozilla.org"], function([a1, a2]) {
    do_check_eq(a1, null);
    do_check_eq(a2, null);

    run_test_3();
  });
}


function run_test_3() {
  MockRegistry.setValue(AM_Ci.nsIWindowsRegKey.ROOT_KEY_LOCAL_MACHINE,
                        "SOFTWARE\\Mozilla\\XPCShell\\Extensions",
                        "addon1@tests.mozilla.org", addon2Dir.path);
  MockRegistry.setValue(AM_Ci.nsIWindowsRegKey.ROOT_KEY_CURRENT_USER,
                        "SOFTWARE\\Mozilla\\XPCShell\\Extensions",
                        "addon2@tests.mozilla.org", addon1Dir.path);

  restartManager();

  AddonManager.getAddonsByIDs(["addon1@tests.mozilla.org",
                               "addon2@tests.mozilla.org"], function([a1, a2]) {
    do_check_eq(a1, null);
    do_check_eq(a2, null);

    
    restartManager();

    run_test_4();
  });
}


function run_test_4() {
  MockRegistry.setValue(AM_Ci.nsIWindowsRegKey.ROOT_KEY_LOCAL_MACHINE,
                        "SOFTWARE\\Mozilla\\XPCShell\\Extensions",
                        "addon1@tests.mozilla.org", null);
  MockRegistry.setValue(AM_Ci.nsIWindowsRegKey.ROOT_KEY_CURRENT_USER,
                        "SOFTWARE\\Mozilla\\XPCShell\\Extensions",
                        "addon2@tests.mozilla.org", null);

  restartManager();

  MockRegistry.setValue(AM_Ci.nsIWindowsRegKey.ROOT_KEY_LOCAL_MACHINE,
                        "SOFTWARE\\Mozilla\\XPCShell\\Extensions",
                        "addon1@tests.mozilla.org", addon1Dir.path);
  restartManager();

  MockRegistry.setValue(AM_Ci.nsIWindowsRegKey.ROOT_KEY_LOCAL_MACHINE,
                        "SOFTWARE\\Mozilla\\XPCShell\\Extensions",
                        "addon1@tests.mozilla.org", null);
  MockRegistry.setValue(AM_Ci.nsIWindowsRegKey.ROOT_KEY_CURRENT_USER,
                        "SOFTWARE\\Mozilla\\XPCShell\\Extensions",
                        "addon2@tests.mozilla.org", addon1Dir.path);
  writeInstallRDFToDir(addon2, addon1Dir);

  restartManager();

  AddonManager.getAddonsByIDs(["addon1@tests.mozilla.org",
                               "addon2@tests.mozilla.org"], function([a1, a2]) {
    do_check_eq(a1, null);
    do_check_neq(a2, null);

    do_test_finished();
  });
}
