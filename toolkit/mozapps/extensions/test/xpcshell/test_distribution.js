







Services.prefs.setBoolPref("extensions.installDistroAddons", true);

createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "1.9.2");

const profileDir = gProfD.clone();
profileDir.append("extensions");
const distroDir = gProfD.clone();
distroDir.append("distribution");
distroDir.append("extensions");
registerDirectory("XREAppDist", distroDir.parent);

var addon1_1 = {
  id: "addon1@tests.mozilla.org",
  version: "1.0",
  name: "Test version 1",
  targetApplications: [{
    id: "xpcshell@tests.mozilla.org",
    minVersion: "1",
    maxVersion: "5"
  }]
};

var addon1_2 = {
  id: "addon1@tests.mozilla.org",
  version: "2.0",
  name: "Test version 2",
  targetApplications: [{
    id: "xpcshell@tests.mozilla.org",
    minVersion: "1",
    maxVersion: "5"
  }]
};

var addon1_3 = {
  id: "addon1@tests.mozilla.org",
  version: "3.0",
  name: "Test version 3",
  targetApplications: [{
    id: "xpcshell@tests.mozilla.org",
    minVersion: "1",
    maxVersion: "5"
  }]
};

function getActiveVersion() {
  return Services.prefs.getIntPref("bootstraptest.active_version");
}

function getInstalledVersion() {
  return Services.prefs.getIntPref("bootstraptest.installed_version");
}

function setOldModificationTime() {
  
  
  shutdownManager()
  let extension = gProfD.clone();
  extension.append("extensions");
  if (Services.prefs.getBoolPref("extensions.alwaysUnpack"))
    extension.append("addon1@tests.mozilla.org");
  else
    extension.append("addon1@tests.mozilla.org.xpi");
  setExtensionModifiedTime(extension, Date.now - 10000);
  startupManager(false);
}

function run_test() {
  do_test_pending();

  run_test_1();
}


function run_test_1() {
  writeInstallRDFForExtension(addon1_1, distroDir);

  startupManager();

  AddonManager.getAddonByID("addon1@tests.mozilla.org", function(a1) {
    do_check_neq(a1, null);
    do_check_eq(a1.version, "1.0");
    do_check_true(a1.isActive);
    do_check_eq(a1.scope, AddonManager.SCOPE_PROFILE);

    run_test_2();
  });
}



function run_test_2() {
  setOldModificationTime();

  writeInstallRDFForExtension(addon1_2, distroDir);

  restartManager();

  AddonManager.getAddonByID("addon1@tests.mozilla.org", function(a1) {
    do_check_neq(a1, null);
    do_check_eq(a1.version, "1.0");
    do_check_true(a1.isActive);
    do_check_eq(a1.scope, AddonManager.SCOPE_PROFILE);

    run_test_3();
  });
}


function run_test_3() {
  restartManager("2");

  AddonManager.getAddonByID("addon1@tests.mozilla.org", function(a1) {
    do_check_neq(a1, null);
    do_check_eq(a1.version, "2.0");
    do_check_true(a1.isActive);
    do_check_eq(a1.scope, AddonManager.SCOPE_PROFILE);

    run_test_4();
  });
}


function run_test_4() {
  setOldModificationTime();

  writeInstallRDFForExtension(addon1_1, distroDir);

  restartManager("3");

  AddonManager.getAddonByID("addon1@tests.mozilla.org", function(a1) {
    do_check_neq(a1, null);
    do_check_eq(a1.version, "2.0");
    do_check_true(a1.isActive);
    do_check_eq(a1.scope, AddonManager.SCOPE_PROFILE);

    run_test_5();
  });
}


function run_test_5() {
  AddonManager.getAddonByID("addon1@tests.mozilla.org", function(a1) {
    a1.uninstall();

    restartManager();

    AddonManager.getAddonByID("addon1@tests.mozilla.org", function(a1) {
      do_check_eq(a1, null);

      run_test_6();
    });
  });
}



function run_test_6() {
  restartManager("4");

  AddonManager.getAddonByID("addon1@tests.mozilla.org", function(a1) {
    do_check_eq(a1, null);

    run_test_7();
  });
}



function run_test_7() {
  Services.prefs.clearUserPref("extensions.installedDistroAddon.addon1@tests.mozilla.org");

  installAllFiles([do_get_addon("test_distribution1_2")], function() {
    restartManager(2);

    AddonManager.getAddonByID("addon1@tests.mozilla.org", function(a1) {
      do_check_neq(a1, null);
      do_check_eq(a1.version, "2.0");
      do_check_true(a1.isActive);
      do_check_eq(a1.scope, AddonManager.SCOPE_PROFILE);

      a1.uninstall();
      restartManager();

      run_test_8();
    });
  });
}



function run_test_8() {
  writeInstallRDFForExtension(addon1_3, distroDir);

  installAllFiles([do_get_addon("test_distribution1_2")], function() {
    restartManager(3);

    AddonManager.getAddonByID("addon1@tests.mozilla.org", function(a1) {
      do_check_neq(a1, null);
      do_check_eq(a1.version, "3.0");
      do_check_true(a1.isActive);
      do_check_eq(a1.scope, AddonManager.SCOPE_PROFILE);

      a1.uninstall();
      restartManager();

      run_test_9();
    });
  });
}



function run_test_9() {
  
  let addon = do_get_file("data/test_distribution2_2");
  addon.copyTo(distroDir, "addon2@tests.mozilla.org");

  restartManager("5");

  AddonManager.getAddonByID("addon2@tests.mozilla.org", function(a2) {
    do_check_neq(a2, null);
    do_check_true(a2.isActive);

    do_check_eq(getInstalledVersion(), 2);
    do_check_eq(getActiveVersion(), 2);

    do_check_true(a2.hasResource("bootstrap.js"));
    do_check_true(a2.hasResource("subdir/dummy.txt"));
    do_check_true(a2.hasResource("subdir/subdir2/dummy2.txt"));

    
    

    let addonDir = profileDir.clone();
    addonDir.append("addon2@tests.mozilla.org");
    do_check_true(addonDir.exists());
    do_check_true(addonDir.isDirectory());
    addonDir.append("subdir");
    do_check_true(addonDir.exists());
    do_check_true(addonDir.isDirectory());
    addonDir.append("subdir2");
    do_check_true(addonDir.exists());
    do_check_true(addonDir.isDirectory());
    addonDir.append("dummy2.txt");
    do_check_true(addonDir.exists());
    do_check_true(addonDir.isFile());

    a2.uninstall();

    do_test_finished();
  });
}
