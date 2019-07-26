






Components.utils.import("resource://gre/modules/osfile.jsm");


var addon1 = {
  id: "addon1@tests.mozilla.org",
  version: "1.0",
  name: "Test 1",
  targetApplications: [{
    id: "xpcshell@tests.mozilla.org",
    minVersion: "2",
    maxVersion: "2"
  }]
};


var addon2 = {
  id: "addon2@tests.mozilla.org",
  version: "1.0",
  name: "Test 2",
  targetApplications: [{
    id: "xpcshell@tests.mozilla.org",
    minVersion: "2",
    maxVersion: "2"
  }]
};


var addon3 = {
  id: "addon3@tests.mozilla.org",
  version: "1.0",
  name: "Test 3",
  targetApplications: [{
    id: "xpcshell@tests.mozilla.org",
    minVersion: "2",
    maxVersion: "2"
  }]
};


var addon4 = {
  id: "addon4@tests.mozilla.org",
  version: "1.0",
  name: "Test 4",
  targetApplications: [{
    id: "xpcshell@tests.mozilla.org",
    minVersion: "2",
    maxVersion: "2"
  }]
};



var addon5 = {
  id: "addon5@tests.mozilla.org",
  version: "1.0",
  name: "Test 5",
  targetApplications: [{
    id: "xpcshell@tests.mozilla.org",
    minVersion: "2",
    maxVersion: "2"
  }]
};

const profileDir = gProfD.clone();
profileDir.append("extensions");

add_task(function() {

  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "2", "2");

  writeInstallRDFForExtension(addon1, profileDir);
  writeInstallRDFForExtension(addon2, profileDir);
  writeInstallRDFForExtension(addon3, profileDir);
  writeInstallRDFForExtension(addon4, profileDir);
  writeInstallRDFForExtension(addon5, profileDir);

  
  
  let path = getFileForAddon(profileDir, addon5.id).path;
  yield promiseSetExtensionModifiedTime(path, Date.now() - (60000));

  
  startupManager();

  check_startup_changes(AddonManager.STARTUP_CHANGE_INSTALLED, []);
  check_startup_changes(AddonManager.STARTUP_CHANGE_UNINSTALLED, []);

  let a1, a2, a3, a4, a5, a6;

  [a2] = yield promiseAddonsByIDs(["addon2@tests.mozilla.org"]);
  a2.userDisabled = true;

  restartManager();

  [a1, a2, a3, a4, a5] =
    yield promiseAddonsByIDs(["addon1@tests.mozilla.org",
                              "addon2@tests.mozilla.org",
                              "addon3@tests.mozilla.org",
                              "addon4@tests.mozilla.org",
                              "addon5@tests.mozilla.org"]);

  a2.userDisabled = false;
  a3.userDisabled = true;
  a4.uninstall();

  yield promiseInstallAllFiles([do_get_addon("test_locked2_5"),
                                do_get_addon("test_locked2_6")]);
  do_check_neq(a1, null);
  do_check_true(a1.isActive);
  do_check_false(a1.userDisabled);
  do_check_false(a1.appDisabled);
  do_check_eq(a1.pendingOperations, AddonManager.PENDING_NONE);
  do_check_true(isExtensionInAddonsList(profileDir, a1.id));

  do_check_neq(a2, null);
  do_check_false(a2.isActive);
  do_check_false(a2.userDisabled);
  do_check_false(a2.appDisabled);
  do_check_eq(a2.pendingOperations, AddonManager.PENDING_ENABLE);
  do_check_false(isExtensionInAddonsList(profileDir, a2.id));

  do_check_neq(a3, null);
  do_check_true(a3.isActive);
  do_check_true(a3.userDisabled);
  do_check_false(a3.appDisabled);
  do_check_eq(a3.pendingOperations, AddonManager.PENDING_DISABLE);
  do_check_true(isExtensionInAddonsList(profileDir, a3.id));

  do_check_neq(a4, null);
  do_check_true(a4.isActive);
  do_check_false(a4.userDisabled);
  do_check_false(a4.appDisabled);
  do_check_eq(a4.pendingOperations, AddonManager.PENDING_UNINSTALL);
  do_check_true(isExtensionInAddonsList(profileDir, a4.id));

  do_check_neq(a5, null);
  do_check_eq(a5.version, "1.0");
  do_check_true(a5.isActive);
  do_check_false(a5.userDisabled);
  do_check_false(a5.appDisabled);
  do_check_eq(a5.pendingOperations, AddonManager.PENDING_UPGRADE);
  do_check_true(isExtensionInAddonsList(profileDir, a5.id));

  
  
  shutdownManager();
  do_print("Locking " + gExtensionsJSON.path);
  let options = {
    winShare: 0
  };
  if (OS.Constants.libc.O_EXLOCK)
    options.unixFlags = OS.Constants.libc.O_EXLOCK;

  let file = yield OS.File.open(gExtensionsJSON.path, {read:true, write:true, existing:true}, options);

  let filePermissions = gExtensionsJSON.permissions;
  if (!OS.Constants.Win) {
    gExtensionsJSON.permissions = 0;
  }
  startupManager(false);

  check_startup_changes(AddonManager.STARTUP_CHANGE_INSTALLED, []);
  check_startup_changes(AddonManager.STARTUP_CHANGE_UNINSTALLED, []);

  [a1, a2, a3, a4, a5, a6] =
    yield promiseAddonsByIDs(["addon1@tests.mozilla.org",
                              "addon2@tests.mozilla.org",
                              "addon3@tests.mozilla.org",
                              "addon4@tests.mozilla.org",
                              "addon5@tests.mozilla.org",
                              "addon6@tests.mozilla.org"]);

  do_check_neq(a1, null);
  do_check_true(a1.isActive);
  do_check_false(a1.userDisabled);
  do_check_false(a1.appDisabled);
  do_check_eq(a1.pendingOperations, AddonManager.PENDING_NONE);
  do_check_true(isExtensionInAddonsList(profileDir, a1.id));

  do_check_neq(a2, null);
  do_check_true(a2.isActive);
  do_check_false(a2.userDisabled);
  do_check_false(a2.appDisabled);
  do_check_eq(a2.pendingOperations, AddonManager.PENDING_NONE);
  do_check_true(isExtensionInAddonsList(profileDir, a2.id));

  do_check_neq(a3, null);
  do_check_false(a3.isActive);
  do_check_true(a3.userDisabled);
  do_check_false(a3.appDisabled);
  do_check_eq(a3.pendingOperations, AddonManager.PENDING_NONE);
  do_check_false(isExtensionInAddonsList(profileDir, a3.id));

  do_check_eq(a4, null);

  do_check_neq(a5, null);
  do_check_eq(a5.version, "2.0");
  do_check_true(a5.isActive);
  do_check_false(a5.userDisabled);
  do_check_false(a5.appDisabled);
  do_check_eq(a5.pendingOperations, AddonManager.PENDING_NONE);
  do_check_true(isExtensionInAddonsList(profileDir, a5.id));

  do_check_neq(a6, null);
  do_check_true(a6.isActive);
  do_check_false(a6.userDisabled);
  do_check_false(a6.appDisabled);
  do_check_eq(a6.pendingOperations, AddonManager.PENDING_NONE);
  do_check_true(isExtensionInAddonsList(profileDir, a6.id));

  
  
  shutdownManager();
  yield file.close();
  gExtensionsJSON.permissions = filePermissions;
  startupManager();

  
  
  
  
  if (gXPISaveError) {
    do_print("Previous XPI save failed");
    check_startup_changes(AddonManager.STARTUP_CHANGE_INSTALLED,
        ["addon6@tests.mozilla.org"]);
    check_startup_changes(AddonManager.STARTUP_CHANGE_UNINSTALLED,
        ["addon4@tests.mozilla.org"]);
  }
  else {
    do_print("Previous XPI save succeeded");
    check_startup_changes(AddonManager.STARTUP_CHANGE_INSTALLED, []);
    check_startup_changes(AddonManager.STARTUP_CHANGE_UNINSTALLED, []);
  }

  [a1, a2, a3, a4, a5, a6] =
    yield promiseAddonsByIDs(["addon1@tests.mozilla.org",
                              "addon2@tests.mozilla.org",
                              "addon3@tests.mozilla.org",
                              "addon4@tests.mozilla.org",
                              "addon5@tests.mozilla.org",
                              "addon6@tests.mozilla.org"]);

  do_check_neq(a1, null);
  do_check_true(a1.isActive);
  do_check_false(a1.userDisabled);
  do_check_false(a1.appDisabled);
  do_check_eq(a1.pendingOperations, AddonManager.PENDING_NONE);
  do_check_true(isExtensionInAddonsList(profileDir, a1.id));

  do_check_neq(a2, null);
  do_check_true(a2.isActive);
  do_check_false(a2.userDisabled);
  do_check_false(a2.appDisabled);
  do_check_eq(a2.pendingOperations, AddonManager.PENDING_NONE);
  do_check_true(isExtensionInAddonsList(profileDir, a2.id));

  do_check_neq(a3, null);
  do_check_false(a3.isActive);
  do_check_true(a3.userDisabled);
  do_check_false(a3.appDisabled);
  do_check_eq(a3.pendingOperations, AddonManager.PENDING_NONE);
  do_check_false(isExtensionInAddonsList(profileDir, a3.id));

  do_check_eq(a4, null);

  do_check_neq(a5, null);
  do_check_eq(a5.version, "2.0");
  do_check_true(a5.isActive);
  do_check_false(a5.userDisabled);
  do_check_false(a5.appDisabled);
  do_check_eq(a5.pendingOperations, AddonManager.PENDING_NONE);
  do_check_true(isExtensionInAddonsList(profileDir, a5.id));

  do_check_neq(a6, null);
  do_check_true(a6.isActive);
  do_check_false(a6.userDisabled);
  do_check_false(a6.appDisabled);
  do_check_eq(a6.pendingOperations, AddonManager.PENDING_NONE);
  do_check_true(isExtensionInAddonsList(profileDir, a6.id));
});

function run_test() {
  run_next_test();
}

