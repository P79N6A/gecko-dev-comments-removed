





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

var addon3 = {
  id: "addon3@tests.mozilla.org",
  version: "2.0",
  name: "Test 3",
  targetApplications: [{
    id: "xpcshell@tests.mozilla.org",
    minVersion: "1",
    maxVersion: "1"
  }]
};

var addon4 = {
  id: "addon4@tests.mozilla.org",
  version: "2.0",
  name: "Test 4",
  targetApplications: [{
    id: "toolkit@mozilla.org",
    minVersion: "1",
    maxVersion: "1"
  }]
};

var addon5 = {
  id: "addon5@tests.mozilla.org",
  version: "2.0",
  name: "Test 5",
  targetApplications: [{
    id: "toolkit@mozilla.org",
    minVersion: "1",
    maxVersion: "1"
  }]
};

var theme1 = {
  id: "theme1@tests.mozilla.org",
  version: "1.0",
  name: "Theme 1",
  type: 4,
  internalName: "theme1/1.0",
  targetApplications: [{
    id: "xpcshell@tests.mozilla.org",
    minVersion: "1",
    maxVersion: "2"
  }]
};

var theme2 = {
  id: "theme2@tests.mozilla.org",
  version: "1.0",
  name: "Theme 2",
  type: 4,
  internalName: "theme2/1.0",
  targetApplications: [{
    id: "xpcshell@tests.mozilla.org",
    minVersion: "1",
    maxVersion: "2"
  }]
};

const profileDir = gProfD.clone();
profileDir.append("extensions");

function run_test() {
  do_test_pending();
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "2", "2");

  writeInstallRDFForExtension(addon1, profileDir);
  writeInstallRDFForExtension(addon2, profileDir);
  writeInstallRDFForExtension(addon3, profileDir);
  writeInstallRDFForExtension(addon4, profileDir);
  writeInstallRDFForExtension(addon5, profileDir);
  writeInstallRDFForExtension(theme1, profileDir);
  writeInstallRDFForExtension(theme2, profileDir);

  let stagedXPIs = profileDir.clone();
  stagedXPIs.append("staged-xpis");
  stagedXPIs.append("addon6@tests.mozilla.org");
  stagedXPIs.create(AM_Ci.nsIFile.DIRECTORY_TYPE, FileUtils.PERMS_DIRECTORY);

  let addon6 = do_get_addon("test_migrate6");
  addon6.copyTo(stagedXPIs, "tmp.xpi");
  stagedXPIs = stagedXPIs.parent;

  stagedXPIs.append("addon7@tests.mozilla.org");
  stagedXPIs.create(AM_Ci.nsIFile.DIRECTORY_TYPE, FileUtils.PERMS_DIRECTORY);

  let addon7 = do_get_addon("test_migrate7");
  addon7.copyTo(stagedXPIs, "tmp.xpi");
  stagedXPIs = stagedXPIs.parent;

  stagedXPIs.append("addon8@tests.mozilla.org");
  stagedXPIs.create(AM_Ci.nsIFile.DIRECTORY_TYPE, FileUtils.PERMS_DIRECTORY);
  let addon8 = do_get_addon("test_migrate8");
  addon8.copyTo(stagedXPIs, "tmp.xpi");
  stagedXPIs = stagedXPIs.parent;

  let old = do_get_file("data/test_migrate.rdf");
  old.copyTo(gProfD, "extensions.rdf");

  let oldCache = gProfD.clone();
  oldCache.append("extensions.cache");
  oldCache.create(AM_Ci.nsIFile.NORMAL_FILE_TYPE, FileUtils.PERMS_FILE);

  
  Services.prefs.setCharPref("general.skins.selectedSkin", "theme1/1.0");

  Services.prefs.setCharPref("extensions.lastAppVersion", "1");

  startupManager();
  check_startup_changes("installed", []);
  check_startup_changes("updated", []);
  check_startup_changes("uninstalled", []);
  check_startup_changes("disabled", []);
  check_startup_changes("enabled", []);

  do_check_false(oldCache.exists());

  AddonManager.getAddonsByIDs(["addon1@tests.mozilla.org",
                               "addon2@tests.mozilla.org",
                               "addon3@tests.mozilla.org",
                               "addon4@tests.mozilla.org",
                               "addon5@tests.mozilla.org",
                               "addon6@tests.mozilla.org",
                               "addon7@tests.mozilla.org",
                               "addon8@tests.mozilla.org",
                               "theme1@tests.mozilla.org",
                               "theme2@tests.mozilla.org"], function([a1, a2, a3,
                                                                      a4, a5, a6,
                                                                      a7, a8, t1,
                                                                      t2]) {
    
    do_check_neq(a1, null);
    do_check_false(a1.userDisabled);
    do_check_false(a1.appDisabled);
    do_check_true(a1.isActive);
    do_check_true(isExtensionInAddonsList(profileDir, a1.id));
    do_check_false(a1.hasBinaryComponents);

    
    do_check_neq(a2, null);
    do_check_true(a2.userDisabled);
    do_check_false(a2.appDisabled);
    do_check_false(a2.isActive);
    do_check_false(isExtensionInAddonsList(profileDir, a2.id));
    do_check_false(a2.hasBinaryComponents);

    
    do_check_neq(a3, null);
    do_check_true(a3.userDisabled);
    do_check_true(a3.appDisabled);
    do_check_false(a3.isActive);
    do_check_false(isExtensionInAddonsList(profileDir, a3.id));
    do_check_false(a3.hasBinaryComponents);

    
    do_check_neq(a4, null);
    do_check_false(a4.userDisabled);
    do_check_true(a4.appDisabled);
    do_check_false(a4.isActive);
    do_check_false(isExtensionInAddonsList(profileDir, a4.id));
    do_check_false(a4.hasBinaryComponents);

    
    
    do_check_neq(a5, null);
    do_check_true(a5.userDisabled);
    do_check_true(a5.appDisabled);
    do_check_false(a5.isActive);
    do_check_false(isExtensionInAddonsList(profileDir, a5.id));
    do_check_false(a5.hasBinaryComponents);

    
    
    do_check_eq(a6, null);
    do_check_eq(a7, null);
    do_check_eq(a8, null);

    
    do_check_neq(t1, null);
    do_check_false(t1.userDisabled);
    do_check_false(t1.appDisabled);
    do_check_true(t1.isActive);
    do_check_true(isThemeInAddonsList(profileDir, t1.id));
    do_check_false(hasFlag(t1.permissions, AddonManager.PERM_CAN_ENABLE));

    
    do_check_neq(t1, null);
    do_check_true(t2.userDisabled);
    do_check_false(t2.appDisabled);
    do_check_false(t2.isActive);
    do_check_false(isThemeInAddonsList(profileDir, t2.id));
    do_check_true(hasFlag(t2.permissions, AddonManager.PERM_CAN_ENABLE));

    do_execute_soon(do_test_finished);
  });
}
