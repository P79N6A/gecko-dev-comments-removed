







Services.prefs.setIntPref("extensions.enabledScopes",
                          AddonManager.SCOPE_PROFILE +
                          AddonManager.SCOPE_APPLICATION);

const profileDir = gProfD.clone();
profileDir.append("extensions");

const globalDir = Services.dirsvc.get("XCurProcD", AM_Ci.nsILocalFile);
globalDir.append("extensions");

var gGlobalExisted = globalDir.exists();
var gInstallTime = Date.now();

function run_test() {
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "1.9.2");

  
  writeInstallRDFForExtension({
    id: "addon1@tests.mozilla.org",
    version: "1.0",
    targetApplications: [{
      id: "xpcshell@tests.mozilla.org",
      minVersion: "1",
      maxVersion: "1"
    }],
    name: "Test Addon 1",
    targetPlatforms: [
      "XPCShell",
      "WINNT_x86",
    ]
  }, profileDir);

  
  writeInstallRDFForExtension({
    id: "addon2@tests.mozilla.org",
    version: "1.0",
    targetApplications: [{
      id: "xpcshell@tests.mozilla.org",
      minVersion: "1",
      maxVersion: "2"
    }],
    name: "Test Addon 2",
    targetPlatforms: [
      "XPCShell_noarch-spidermonkey"
    ]
  }, profileDir);

  
  writeInstallRDFForExtension({
    id: "addon3@tests.mozilla.org",
    version: "1.0",
    targetApplications: [{
      id: "xpcshell@tests.mozilla.org",
      minVersion: "2",
      maxVersion: "2"
    }],
    name: "Test Addon 3",
  }, profileDir);

  
  var dest = writeInstallRDFForExtension({
    id: "addon4@tests.mozilla.org",
    version: "1.0",
    targetApplications: [{
      id: "xpcshell@tests.mozilla.org",
      minVersion: "1",
      maxVersion: "1"
    }],
    name: "Test Addon 4",
  }, globalDir);
  setExtensionModifiedTime(dest, gInstallTime);

  do_test_pending();

  Services.prefs.setBoolPref(PREF_EM_STRICT_COMPATIBILITY, true);

  run_test_1();
}

function end_test() {
  if (!gGlobalExisted) {
    globalDir.remove(true);
  }
  else {
    globalDir.append(do_get_expected_addon_name("addon4@tests.mozilla.org"));
    globalDir.remove(true);
  }

  Services.prefs.clearUserPref(PREF_EM_STRICT_COMPATIBILITY);

  do_test_finished();
}


function run_test_1() {
  startupManager();

  AddonManager.getAddonsByIDs(["addon1@tests.mozilla.org",
                               "addon2@tests.mozilla.org",
                               "addon3@tests.mozilla.org",
                               "addon4@tests.mozilla.org"],
                               function([a1, a2, a3, a4]) {

    do_check_neq(a1, null);
    do_check_true(isExtensionInAddonsList(profileDir, a1.id));

    do_check_neq(a2, null);
    do_check_true(isExtensionInAddonsList(profileDir, a2.id));

    do_check_neq(a3, null);
    do_check_false(isExtensionInAddonsList(profileDir, a3.id));

    do_check_neq(a4, null);
    do_check_true(isExtensionInAddonsList(globalDir, a4.id));
    do_check_eq(a4.version, "1.0");

    run_test_2();
  });
}


function run_test_2() {
  
  var dest = writeInstallRDFForExtension({
    id: "addon4@tests.mozilla.org",
    version: "2.0",
    targetApplications: [{
      id: "xpcshell@tests.mozilla.org",
      minVersion: "2",
      maxVersion: "2"
    }],
    name: "Test Addon 4",
  }, globalDir);
  setExtensionModifiedTime(dest, gInstallTime);

  restartManager("2");
  AddonManager.getAddonsByIDs(["addon1@tests.mozilla.org",
                               "addon2@tests.mozilla.org",
                               "addon3@tests.mozilla.org",
                               "addon4@tests.mozilla.org"],
                               function([a1, a2, a3, a4]) {

    do_check_neq(a1, null);
    do_check_false(isExtensionInAddonsList(profileDir, a1.id));

    do_check_neq(a2, null);
    do_check_true(isExtensionInAddonsList(profileDir, a2.id));

    do_check_neq(a3, null);
    do_check_true(isExtensionInAddonsList(profileDir, a3.id));

    do_check_neq(a4, null);
    do_check_true(isExtensionInAddonsList(globalDir, a4.id));
    do_check_eq(a4.version, "2.0");

    run_test_3();
  });
}


function run_test_3() {
  
  var dest = writeInstallRDFForExtension({
    id: "addon4@tests.mozilla.org",
    version: "3.0",
    targetApplications: [{
      id: "xpcshell@tests.mozilla.org",
      minVersion: "3",
      maxVersion: "3"
    }],
    name: "Test Addon 4",
  }, globalDir);
  setExtensionModifiedTime(dest, gInstallTime);

  
  
  var file = gProfD.clone();
  file.append("extensions.ini");
  file.remove(true);
  restartManager();

  AddonManager.getAddonsByIDs(["addon1@tests.mozilla.org",
                               "addon2@tests.mozilla.org",
                               "addon3@tests.mozilla.org",
                               "addon4@tests.mozilla.org"],
                               function([a1, a2, a3, a4]) {

    do_check_neq(a1, null);
    do_check_false(isExtensionInAddonsList(profileDir, a1.id));

    do_check_neq(a2, null);
    do_check_true(isExtensionInAddonsList(profileDir, a2.id));

    do_check_neq(a3, null);
    do_check_true(isExtensionInAddonsList(profileDir, a3.id));

    do_check_neq(a4, null);
    do_check_true(isExtensionInAddonsList(globalDir, a4.id));
    do_check_eq(a4.version, "2.0");

    end_test();
  });
}
