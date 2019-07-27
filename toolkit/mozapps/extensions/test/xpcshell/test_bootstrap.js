



const APP_STARTUP                     = 1;
const APP_SHUTDOWN                    = 2;
const ADDON_ENABLE                    = 3;
const ADDON_DISABLE                   = 4;
const ADDON_INSTALL                   = 5;
const ADDON_UNINSTALL                 = 6;
const ADDON_UPGRADE                   = 7;
const ADDON_DOWNGRADE                 = 8;


Components.utils.import("resource://gre/modules/Services.jsm");
Components.utils.import("resource://gre/modules/Promise.jsm");


Services.prefs.setIntPref("extensions.enabledScopes",
                          AddonManager.SCOPE_PROFILE + AddonManager.SCOPE_USER);

createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "1.9.2");

const profileDir = gProfD.clone();
profileDir.append("extensions");
const userExtDir = gProfD.clone();
userExtDir.append("extensions2");
userExtDir.append(gAppInfo.ID);
registerDirectory("XREUSysExt", userExtDir.parent);

Components.utils.import("resource://testing-common/httpd.js");
var testserver = new HttpServer();
testserver.start(-1);
gPort = testserver.identity.primaryPort;

testserver.registerDirectory("/addons/", do_get_file("addons"));

function resetPrefs() {
  Services.prefs.setIntPref("bootstraptest.active_version", -1);
  Services.prefs.setIntPref("bootstraptest.installed_version", -1);
  Services.prefs.setIntPref("bootstraptest2.active_version", -1);
  Services.prefs.setIntPref("bootstraptest2.installed_version", -1);
  Services.prefs.setIntPref("bootstraptest.startup_reason", -1);
  Services.prefs.setIntPref("bootstraptest.shutdown_reason", -1);
  Services.prefs.setIntPref("bootstraptest.install_reason", -1);
  Services.prefs.setIntPref("bootstraptest.uninstall_reason", -1);
  Services.prefs.setIntPref("bootstraptest.startup_oldversion", -1);
  Services.prefs.setIntPref("bootstraptest.shutdown_newversion", -1);
  Services.prefs.setIntPref("bootstraptest.install_oldversion", -1);
  Services.prefs.setIntPref("bootstraptest.uninstall_newversion", -1);
}

function waitForPref(aPref, aCallback) {
  function prefChanged() {
    Services.prefs.removeObserver(aPref, prefChanged);
    
    do_execute_soon(aCallback);
  }
  Services.prefs.addObserver(aPref, prefChanged, false);
}

function promisePref(aPref) {
  let deferred = Promise.defer();

  waitForPref(aPref, deferred.resolve.bind(deferred));

  return deferred.promise;
}

function promiseInstall(aFiles) {
  let deferred = Promise.defer();

  installAllFiles(aFiles, function() {
    deferred.resolve();
  });

  return deferred.promise;
}

function getActiveVersion() {
  return Services.prefs.getIntPref("bootstraptest.active_version");
}

function getInstalledVersion() {
  return Services.prefs.getIntPref("bootstraptest.installed_version");
}

function getActiveVersion2() {
  return Services.prefs.getIntPref("bootstraptest2.active_version");
}

function getInstalledVersion2() {
  return Services.prefs.getIntPref("bootstraptest2.installed_version");
}

function getStartupReason() {
  return Services.prefs.getIntPref("bootstraptest.startup_reason");
}

function getShutdownReason() {
  return Services.prefs.getIntPref("bootstraptest.shutdown_reason");
}

function getInstallReason() {
  return Services.prefs.getIntPref("bootstraptest.install_reason");
}

function getUninstallReason() {
  return Services.prefs.getIntPref("bootstraptest.uninstall_reason");
}

function getStartupOldVersion() {
  return Services.prefs.getIntPref("bootstraptest.startup_oldversion");
}

function getShutdownNewVersion() {
  return Services.prefs.getIntPref("bootstraptest.shutdown_newversion");
}

function getInstallOldVersion() {
  return Services.prefs.getIntPref("bootstraptest.install_oldversion");
}

function getUninstallNewVersion() {
  return Services.prefs.getIntPref("bootstraptest.uninstall_newversion");
}

function do_check_bootstrappedPref(aCallback) {
  let data = Services.prefs.getCharPref("extensions.bootstrappedAddons");
  data = JSON.parse(data);

  AddonManager.getAddonsByTypes(["extension"], function(aAddons) {
    for (let addon of aAddons) {
      if (!addon.id.endsWith("@tests.mozilla.org"))
        continue;
      if (!addon.isActive)
        continue;
      if (addon.operationsRequiringRestart != AddonManager.OP_NEEDS_RESTART_NONE)
        continue;

      do_check_true(addon.id in data);
      let addonData = data[addon.id];
      delete data[addon.id];

      do_check_eq(addonData.version, addon.version);
      do_check_eq(addonData.type, addon.type);
      let file = addon.getResourceURI().QueryInterface(Components.interfaces.nsIFileURL).file;
      do_check_eq(addonData.descriptor, file.persistentDescriptor);
    }
    do_check_eq(Object.keys(data).length, 0);

    do_execute_soon(aCallback);
  });
}


function run_test() {
  do_test_pending();

  resetPrefs();

  startupManager();

  do_check_false(gExtensionsJSON.exists());

  do_check_false(gExtensionsINI.exists());

  run_test_1();
}


function run_test_1() {
  prepare_test({ }, [
    "onNewInstall"
  ]);

  AddonManager.getInstallForFile(do_get_addon("test_bootstrap1_1"), function(install) {
    ensure_test_completed();

    do_check_neq(install, null);
    do_check_eq(install.type, "extension");
    do_check_eq(install.version, "1.0");
    do_check_eq(install.name, "Test Bootstrap 1");
    do_check_eq(install.state, AddonManager.STATE_DOWNLOADED);
    do_check_neq(install.addon.syncGUID, null);
    do_check_true(install.addon.hasResource("install.rdf"));
    do_check_true(install.addon.hasResource("bootstrap.js"));
    do_check_false(install.addon.hasResource("foo.bar"));
    do_check_eq(install.addon.operationsRequiringRestart &
                AddonManager.OP_NEEDS_RESTART_INSTALL, 0);
    do_check_not_in_crash_annotation("bootstrap1@tests.mozilla.org", "1.0");

    let addon = install.addon;

    waitForPref("bootstraptest.startup_reason", function() {
      do_check_bootstrappedPref(function() {
        check_test_1(addon.syncGUID);
      });
    });

    prepare_test({
      "bootstrap1@tests.mozilla.org": [
        ["onInstalling", false],
        "onInstalled"
      ]
    }, [
      "onInstallStarted",
      "onInstallEnded",
    ], function() {
      do_check_true(addon.hasResource("install.rdf"));

      
      do_check_eq(getActiveVersion(), -1);
    });
    install.install();
  });
}

function check_test_1(installSyncGUID) {
  do_check_false(gExtensionsINI.exists());

  AddonManager.getAllInstalls(function(installs) {
    
    
    do_check_eq(installs.length, 0);

    AddonManager.getAddonByID("bootstrap1@tests.mozilla.org", function(b1) {
      do_check_neq(b1, null);
      do_check_eq(b1.version, "1.0");
      do_check_neq(b1.syncGUID, null);
      do_check_eq(b1.syncGUID, installSyncGUID);
      do_check_false(b1.appDisabled);
      do_check_false(b1.userDisabled);
      do_check_true(b1.isActive);
      do_check_eq(getInstalledVersion(), 1);
      do_check_eq(getActiveVersion(), 1);
      do_check_eq(getStartupReason(), ADDON_INSTALL);
      do_check_eq(getStartupOldVersion(), 0);
      do_check_true(b1.hasResource("install.rdf"));
      do_check_true(b1.hasResource("bootstrap.js"));
      do_check_false(b1.hasResource("foo.bar"));
      do_check_in_crash_annotation("bootstrap1@tests.mozilla.org", "1.0");

      let dir = do_get_addon_root_uri(profileDir, "bootstrap1@tests.mozilla.org");
      do_check_eq(b1.getResourceURI("bootstrap.js").spec, dir + "bootstrap.js");

      AddonManager.getAddonsWithOperationsByTypes(null, function(list) {
        do_check_eq(list.length, 0);

        do_execute_soon(run_test_2);
      });
    });
  });
}


function run_test_2() {
  AddonManager.getAddonByID("bootstrap1@tests.mozilla.org", function(b1) {
    prepare_test({
      "bootstrap1@tests.mozilla.org": [
        ["onDisabling", false],
        "onDisabled"
      ]
    });

    do_check_eq(b1.operationsRequiringRestart &
                AddonManager.OP_NEEDS_RESTART_DISABLE, 0);
    b1.userDisabled = true;
    ensure_test_completed();

    do_check_neq(b1, null);
    do_check_eq(b1.version, "1.0");
    do_check_false(b1.appDisabled);
    do_check_true(b1.userDisabled);
    do_check_false(b1.isActive);
    do_check_eq(getInstalledVersion(), 1);
    do_check_eq(getActiveVersion(), 0);
    do_check_eq(getShutdownReason(), ADDON_DISABLE);
    do_check_eq(getShutdownNewVersion(), 0);
    do_check_not_in_crash_annotation("bootstrap1@tests.mozilla.org", "1.0");

    AddonManager.getAddonByID("bootstrap1@tests.mozilla.org", function(newb1) {
      do_check_neq(newb1, null);
      do_check_eq(newb1.version, "1.0");
      do_check_false(newb1.appDisabled);
      do_check_true(newb1.userDisabled);
      do_check_false(newb1.isActive);

      do_check_bootstrappedPref(run_test_3);
    });
  });
}


function run_test_3() {
  shutdownManager();
  do_check_eq(getInstalledVersion(), 1);
  do_check_eq(getActiveVersion(), 0);
  do_check_eq(getShutdownReason(), ADDON_DISABLE);
  do_check_eq(getShutdownNewVersion(), 0);
  startupManager(false);
  do_check_eq(getInstalledVersion(), 1);
  do_check_eq(getActiveVersion(), 0);
  do_check_eq(getShutdownReason(), ADDON_DISABLE);
  do_check_eq(getShutdownNewVersion(), 0);
  do_check_not_in_crash_annotation("bootstrap1@tests.mozilla.org", "1.0");

  do_check_false(gExtensionsINI.exists());

  AddonManager.getAddonByID("bootstrap1@tests.mozilla.org", function(b1) {
    do_check_neq(b1, null);
    do_check_eq(b1.version, "1.0");
    do_check_false(b1.appDisabled);
    do_check_true(b1.userDisabled);
    do_check_false(b1.isActive);

    do_check_bootstrappedPref(run_test_4);
  });
}


function run_test_4() {
  AddonManager.getAddonByID("bootstrap1@tests.mozilla.org", function(b1) {
    prepare_test({
      "bootstrap1@tests.mozilla.org": [
        ["onEnabling", false],
        "onEnabled"
      ]
    });

    do_check_eq(b1.operationsRequiringRestart &
                AddonManager.OP_NEEDS_RESTART_ENABLE, 0);
    b1.userDisabled = false;
    ensure_test_completed();

    do_check_neq(b1, null);
    do_check_eq(b1.version, "1.0");
    do_check_false(b1.appDisabled);
    do_check_false(b1.userDisabled);
    do_check_true(b1.isActive);
    do_check_eq(getInstalledVersion(), 1);
    do_check_eq(getActiveVersion(), 1);
    do_check_eq(getStartupReason(), ADDON_ENABLE);
    do_check_eq(getStartupOldVersion(), 0);
    do_check_in_crash_annotation("bootstrap1@tests.mozilla.org", "1.0");

    AddonManager.getAddonByID("bootstrap1@tests.mozilla.org", function(newb1) {
      do_check_neq(newb1, null);
      do_check_eq(newb1.version, "1.0");
      do_check_false(newb1.appDisabled);
      do_check_false(newb1.userDisabled);
      do_check_true(newb1.isActive);

      do_check_bootstrappedPref(run_test_5);
    });
  });
}


function run_test_5() {
  shutdownManager();
  
  do_check_true(gExtensionsJSON.exists());

  do_check_eq(getInstalledVersion(), 1);
  do_check_eq(getActiveVersion(), 0);
  do_check_eq(getShutdownReason(), APP_SHUTDOWN);
  do_check_eq(getShutdownNewVersion(), 0);
  do_check_not_in_crash_annotation("bootstrap1@tests.mozilla.org", "1.0");
  startupManager(false);
  do_check_eq(getInstalledVersion(), 1);
  do_check_eq(getActiveVersion(), 1);
  do_check_eq(getStartupReason(), APP_STARTUP);
  do_check_eq(getStartupOldVersion(), 0);
  do_check_in_crash_annotation("bootstrap1@tests.mozilla.org", "1.0");

  AddonManager.getAddonByID("bootstrap1@tests.mozilla.org", function(b1) {
    do_check_neq(b1, null);
    do_check_eq(b1.version, "1.0");
    do_check_false(b1.appDisabled);
    do_check_false(b1.userDisabled);
    do_check_true(b1.isActive);
    do_check_false(isExtensionInAddonsList(profileDir, b1.id));

    do_check_bootstrappedPref(run_test_6);
  });
}


function run_test_6() {
  prepare_test({ }, [
    "onNewInstall"
  ]);

  AddonManager.getInstallForFile(do_get_addon("test_bootstrap1_2"), function(install) {
    ensure_test_completed();

    do_check_neq(install, null);
    do_check_eq(install.type, "extension");
    do_check_eq(install.version, "2.0");
    do_check_eq(install.name, "Test Bootstrap 1");
    do_check_eq(install.state, AddonManager.STATE_DOWNLOADED);

    waitForPref("bootstraptest.startup_reason", check_test_6);
    prepare_test({
      "bootstrap1@tests.mozilla.org": [
        ["onInstalling", false],
        "onInstalled"
      ]
    }, [
      "onInstallStarted",
      "onInstallEnded",
    ], function() {
    });
    install.install();
  });
}

function check_test_6() {
  AddonManager.getAddonByID("bootstrap1@tests.mozilla.org", function(b1) {
    do_check_neq(b1, null);
    do_check_eq(b1.version, "2.0");
    do_check_false(b1.appDisabled);
    do_check_false(b1.userDisabled);
    do_check_true(b1.isActive);
    do_check_eq(getInstalledVersion(), 2);
    do_check_eq(getActiveVersion(), 2);
    do_check_eq(getStartupReason(), ADDON_UPGRADE);
    do_check_eq(getInstallOldVersion(), 1);
    do_check_eq(getStartupOldVersion(), 1);
    do_check_eq(getShutdownReason(), ADDON_UPGRADE);
    do_check_eq(getShutdownNewVersion(), 2);
    do_check_eq(getUninstallNewVersion(), 2);
    do_check_not_in_crash_annotation("bootstrap1@tests.mozilla.org", "1.0");
    do_check_in_crash_annotation("bootstrap1@tests.mozilla.org", "2.0");

    do_check_bootstrappedPref(run_test_7);
  });
}


function run_test_7() {
  AddonManager.getAddonByID("bootstrap1@tests.mozilla.org", function(b1) {
    prepare_test({
      "bootstrap1@tests.mozilla.org": [
        ["onUninstalling", false],
        "onUninstalled"
      ]
    });

    do_check_eq(b1.operationsRequiringRestart &
                AddonManager.OP_NEEDS_RESTART_UNINSTALL, 0);
    b1.uninstall();

    do_check_bootstrappedPref(check_test_7);
  });
}

function check_test_7() {
  ensure_test_completed();
  do_check_eq(getInstalledVersion(), 0);
  do_check_eq(getActiveVersion(), 0);
  do_check_eq(getShutdownReason(), ADDON_UNINSTALL);
  do_check_eq(getShutdownNewVersion(), 0);
  do_check_not_in_crash_annotation("bootstrap1@tests.mozilla.org", "2.0");

  AddonManager.getAddonByID("bootstrap1@tests.mozilla.org", callback_soon(function(b1) {
    do_check_eq(b1, null);

    restartManager();

    AddonManager.getAddonByID("bootstrap1@tests.mozilla.org", function(newb1) {
      do_check_eq(newb1, null);

      do_check_bootstrappedPref(run_test_8);
    });
  }));
}



function run_test_8() {
  shutdownManager();

  manuallyInstall(do_get_addon("test_bootstrap1_1"), profileDir,
                  "bootstrap1@tests.mozilla.org");

  startupManager(false);

  AddonManager.getAddonByID("bootstrap1@tests.mozilla.org", function(b1) {
    do_check_neq(b1, null);
    do_check_eq(b1.version, "1.0");
    do_check_false(b1.appDisabled);
    do_check_false(b1.userDisabled);
    do_check_true(b1.isActive);
    do_check_eq(getInstalledVersion(), 1);
    do_check_eq(getActiveVersion(), 1);
    do_check_eq(getStartupReason(), ADDON_INSTALL);
    do_check_eq(getStartupOldVersion(), 0);
    do_check_in_crash_annotation("bootstrap1@tests.mozilla.org", "1.0");

    do_check_bootstrappedPref(run_test_9);
  });
}


function run_test_9() {
  shutdownManager();

  manuallyUninstall(profileDir, "bootstrap1@tests.mozilla.org");

  startupManager(false);

  AddonManager.getAddonByID("bootstrap1@tests.mozilla.org", function(b1) {
    do_check_eq(b1, null);
    do_check_not_in_crash_annotation("bootstrap1@tests.mozilla.org", "1.0");

    do_check_bootstrappedPref(run_test_10);
  });
}



function run_test_10() {
  resetPrefs();
  prepare_test({ }, [
    "onNewInstall"
  ]);

  AddonManager.getInstallForFile(do_get_addon("test_bootstrap1_2"), function(install) {
    ensure_test_completed();

    do_check_neq(install, null);
    do_check_eq(install.type, "extension");
    do_check_eq(install.version, "2.0");
    do_check_eq(install.name, "Test Bootstrap 1");
    do_check_eq(install.state, AddonManager.STATE_DOWNLOADED);
    do_check_true(install.addon.hasResource("install.rdf"));
    do_check_true(install.addon.hasResource("bootstrap.js"));
    do_check_false(install.addon.hasResource("foo.bar"));
    do_check_not_in_crash_annotation("bootstrap1@tests.mozilla.org", "2.0");

    waitForPref("bootstraptest.startup_reason", check_test_10_pt1);
    prepare_test({
      "bootstrap1@tests.mozilla.org": [
        ["onInstalling", false],
        "onInstalled"
      ]
    }, [
      "onInstallStarted",
      "onInstallEnded",
    ], function() {
      do_print("Waiting for startup of bootstrap1_2");
    });
    install.install();
  });
}

function check_test_10_pt1() {
  AddonManager.getAddonByID("bootstrap1@tests.mozilla.org", function(b1) {
    do_check_neq(b1, null);
    do_check_eq(b1.version, "2.0");
    do_check_false(b1.appDisabled);
    do_check_false(b1.userDisabled);
    do_check_true(b1.isActive);
    do_check_eq(getInstalledVersion(), 2);
    do_check_eq(getActiveVersion(), 2);
    do_check_eq(getStartupReason(), ADDON_INSTALL);
    do_check_eq(getStartupOldVersion(), 0);
    do_check_true(b1.hasResource("install.rdf"));
    do_check_true(b1.hasResource("bootstrap.js"));
    do_check_false(b1.hasResource("foo.bar"));
    do_check_in_crash_annotation("bootstrap1@tests.mozilla.org", "2.0");

    prepare_test({ }, [
      "onNewInstall"
    ]);

    AddonManager.getInstallForFile(do_get_addon("test_bootstrap1_1"), function(install) {
      ensure_test_completed();

      do_check_neq(install, null);
      do_check_eq(install.type, "extension");
      do_check_eq(install.version, "1.0");
      do_check_eq(install.name, "Test Bootstrap 1");
      do_check_eq(install.state, AddonManager.STATE_DOWNLOADED);

      waitForPref("bootstraptest.startup_reason", check_test_10_pt2);
      prepare_test({
        "bootstrap1@tests.mozilla.org": [
          ["onInstalling", false],
          "onInstalled"
        ]
      }, [
        "onInstallStarted",
        "onInstallEnded",
      ], function() { });
      install.install();
    });
  });
}

function check_test_10_pt2() {
  AddonManager.getAddonByID("bootstrap1@tests.mozilla.org", function(b1) {
    do_check_neq(b1, null);
    do_check_eq(b1.version, "1.0");
    do_check_false(b1.appDisabled);
    do_check_false(b1.userDisabled);
    do_check_true(b1.isActive);
    do_check_eq(getInstalledVersion(), 1);
    do_check_eq(getActiveVersion(), 1);
    do_check_eq(getStartupReason(), ADDON_DOWNGRADE);
    do_check_eq(getInstallOldVersion(), 2);
    do_check_eq(getStartupOldVersion(), 2);
    do_check_eq(getShutdownReason(), ADDON_DOWNGRADE);
    do_check_eq(getShutdownNewVersion(), 1);
    do_check_eq(getUninstallNewVersion(), 1);
    do_check_in_crash_annotation("bootstrap1@tests.mozilla.org", "1.0");
    do_check_not_in_crash_annotation("bootstrap1@tests.mozilla.org", "2.0");

    do_check_bootstrappedPref(run_test_11);
  });
}


function run_test_11() {
  AddonManager.getAddonByID("bootstrap1@tests.mozilla.org", function(b1) {
    prepare_test({
      "bootstrap1@tests.mozilla.org": [
        ["onDisabling", false],
        "onDisabled",
        ["onUninstalling", false],
        "onUninstalled"
      ]
    });

    b1.userDisabled = true;

    do_check_eq(getInstalledVersion(), 1);
    do_check_eq(getActiveVersion(), 0);
    do_check_eq(getShutdownReason(), ADDON_DISABLE);
    do_check_eq(getShutdownNewVersion(), 0);
    do_check_not_in_crash_annotation("bootstrap1@tests.mozilla.org", "1.0");

    b1.uninstall();

    check_test_11();
  });
}

function check_test_11() {
  ensure_test_completed();
  do_check_eq(getInstalledVersion(), 0);
  do_check_eq(getActiveVersion(), 0);
  do_check_not_in_crash_annotation("bootstrap1@tests.mozilla.org", "1.0");

  do_check_bootstrappedPref(run_test_12);
}



function run_test_12() {
  shutdownManager();

  manuallyInstall(do_get_addon("test_bootstrap1_1"), profileDir,
                  "bootstrap1@tests.mozilla.org");

  startupManager(true);

  AddonManager.getAddonByID("bootstrap1@tests.mozilla.org", function(b1) {
    do_check_neq(b1, null);
    do_check_eq(b1.version, "1.0");
    do_check_false(b1.appDisabled);
    do_check_false(b1.userDisabled);
    do_check_true(b1.isActive);
    do_check_eq(getInstalledVersion(), 1);
    do_check_eq(getActiveVersion(), 1);
    do_check_eq(getStartupReason(), ADDON_INSTALL);
    do_check_eq(getStartupOldVersion(), 0);
    do_check_in_crash_annotation("bootstrap1@tests.mozilla.org", "1.0");

    b1.uninstall();
    do_execute_soon(test_12_restart);
  });
}

function test_12_restart() {
  restartManager();
  do_check_bootstrappedPref(run_test_13);
}




function run_test_13() {
  prepare_test({ }, [
    "onNewInstall"
  ]);

  AddonManager.getInstallForFile(do_get_addon("test_bootstrap1_3"), function(install) {
    ensure_test_completed();

    do_check_neq(install, null);
    do_check_eq(install.type, "extension");
    do_check_eq(install.version, "3.0");
    do_check_eq(install.name, "Test Bootstrap 1");
    do_check_eq(install.state, AddonManager.STATE_DOWNLOADED);
    do_check_not_in_crash_annotation("bootstrap1@tests.mozilla.org", "3.0");

    prepare_test({
      "bootstrap1@tests.mozilla.org": [
        ["onInstalling", false],
        "onInstalled"
      ]
    }, [
      "onInstallStarted",
      "onInstallEnded",
    ], callback_soon(check_test_13));
    install.install();
  });
}

function check_test_13() {
  AddonManager.getAllInstalls(function(installs) {
    
    
    do_check_eq(installs.length, 0);

    AddonManager.getAddonByID("bootstrap1@tests.mozilla.org", function(b1) {
      do_check_neq(b1, null);
      do_check_eq(b1.version, "3.0");
      do_check_true(b1.appDisabled);
      do_check_false(b1.userDisabled);
      do_check_false(b1.isActive);
      do_check_eq(getInstalledVersion(), 3);  
      do_check_eq(getActiveVersion(), 0);     
      do_check_not_in_crash_annotation("bootstrap1@tests.mozilla.org", "3.0");

      do_execute_soon(test_13_restart);
    });
  });
}

function test_13_restart() {
  restartManager();

  AddonManager.getAddonByID("bootstrap1@tests.mozilla.org", function(b1) {
    do_check_neq(b1, null);
    do_check_eq(b1.version, "3.0");
    do_check_true(b1.appDisabled);
    do_check_false(b1.userDisabled);
    do_check_false(b1.isActive);
    do_check_eq(getInstalledVersion(), 3);  
    do_check_eq(getActiveVersion(), 0);     
    do_check_not_in_crash_annotation("bootstrap1@tests.mozilla.org", "3.0");

    do_check_bootstrappedPref(function() {
      b1.uninstall();
      do_execute_soon(run_test_14);
    });
  });
}



function run_test_14() {
  restartManager();

  shutdownManager();

  manuallyInstall(do_get_addon("test_bootstrap1_3"), profileDir,
                  "bootstrap1@tests.mozilla.org");

  startupManager(false);

  AddonManager.getAddonByID("bootstrap1@tests.mozilla.org", function(b1) {
    do_check_neq(b1, null);
    do_check_eq(b1.version, "3.0");
    do_check_true(b1.appDisabled);
    do_check_false(b1.userDisabled);
    do_check_false(b1.isActive);
    do_check_eq(getInstalledVersion(), 3);   
    do_check_eq(getActiveVersion(), 0);      
    do_check_not_in_crash_annotation("bootstrap1@tests.mozilla.org", "3.0");

    do_check_bootstrappedPref(function() {
      b1.uninstall();

      run_test_15();
    });
  });
}



function run_test_15() {
  resetPrefs();
  waitForPref("bootstraptest.startup_reason", function test_15_after_startup() {
    AddonManager.getAddonByID("bootstrap1@tests.mozilla.org", function(b1) {
      do_check_neq(b1, null);
      do_check_eq(b1.version, "1.0");
      do_check_false(b1.appDisabled);
      do_check_false(b1.userDisabled);
      do_check_true(b1.isActive);
      do_check_eq(getInstalledVersion(), 1);
      do_check_eq(getActiveVersion(), 1);

      b1.userDisabled = true;
      do_check_false(b1.isActive);
      do_check_eq(getInstalledVersion(), 1);
      do_check_eq(getActiveVersion(), 0);

      prepare_test({ }, [
        "onNewInstall"
      ]);

      AddonManager.getInstallForFile(do_get_addon("test_bootstrap1_2"), function(install) {
        ensure_test_completed();

        do_check_neq(install, null);
        do_check_true(install.addon.userDisabled);

        prepare_test({
          "bootstrap1@tests.mozilla.org": [
            ["onInstalling", false],
            "onInstalled"
          ]
        }, [
          "onInstallStarted",
          "onInstallEnded",
        ], callback_soon(check_test_15));
        install.install();
      });
    });
  });
  installAllFiles([do_get_addon("test_bootstrap1_1")], function test_15_addon_installed() { });
}

function check_test_15() {
  AddonManager.getAddonByID("bootstrap1@tests.mozilla.org", function(b1) {
    do_check_neq(b1, null);
    do_check_eq(b1.version, "2.0");
    do_check_false(b1.appDisabled);
    do_check_true(b1.userDisabled);
    do_check_false(b1.isActive);
    do_check_eq(getInstalledVersion(), 2);
    do_check_eq(getActiveVersion(), 0);

    do_check_bootstrappedPref(function() {
      restartManager();

      AddonManager.getAddonByID("bootstrap1@tests.mozilla.org", callback_soon(function(b1) {
        do_check_neq(b1, null);
        do_check_eq(b1.version, "2.0");
        do_check_false(b1.appDisabled);
        do_check_true(b1.userDisabled);
        do_check_false(b1.isActive);
        do_check_eq(getInstalledVersion(), 2);
        do_check_eq(getActiveVersion(), 0);

        b1.uninstall();

        run_test_16();
      }));
    });
  });
}


function run_test_16() {
  resetPrefs();
  waitForPref("bootstraptest.startup_reason", function test_16_after_startup() {
    AddonManager.getAddonByID("bootstrap1@tests.mozilla.org", callback_soon(function(b1) {
      
      do_check_eq(getInstalledVersion(), 1);
      do_check_eq(getActiveVersion(), 1);
      do_check_true(b1.isActive);
      do_check_eq(b1.iconURL, "chrome://foo/skin/icon.png");
      do_check_eq(b1.aboutURL, "chrome://foo/content/about.xul");
      do_check_eq(b1.optionsURL, "chrome://foo/content/options.xul");

      shutdownManager();

      
      do_check_eq(getInstalledVersion(), 1);
      do_check_eq(getActiveVersion(), 0);

      gAppInfo.inSafeMode = true;
      startupManager(false);

      AddonManager.getAddonByID("bootstrap1@tests.mozilla.org", callback_soon(function(b1) {
        
        do_check_eq(getInstalledVersion(), 1);
        do_check_eq(getActiveVersion(), 0);
        do_check_false(b1.isActive);
        do_check_eq(b1.iconURL, null);
        do_check_eq(b1.aboutURL, null);
        do_check_eq(b1.optionsURL, null);

        shutdownManager();
        gAppInfo.inSafeMode = false;
        startupManager(false);

        
        do_check_eq(getInstalledVersion(), 1);
        do_check_eq(getActiveVersion(), 1);

        AddonManager.getAddonByID("bootstrap1@tests.mozilla.org", function(b1) {
          b1.uninstall();

          do_execute_soon(run_test_17);
        });
      }));
    }));
  });
  installAllFiles([do_get_addon("test_bootstrap1_1")], function() { });
}


function run_test_17() {
  shutdownManager();

  manuallyInstall(do_get_addon("test_bootstrap1_1"), userExtDir,
                  "bootstrap1@tests.mozilla.org");

  resetPrefs();
  startupManager();

  AddonManager.getAddonByID("bootstrap1@tests.mozilla.org", function(b1) {
    
    do_check_eq(getInstalledVersion(), 1);
    do_check_eq(getActiveVersion(), 1);
    do_check_neq(b1, null);
    do_check_eq(b1.version, "1.0");
    do_check_true(b1.isActive);

    do_check_bootstrappedPref(run_test_18);
  });
}



function run_test_18() {
  resetPrefs();
  waitForPref("bootstraptest.startup_reason", function test_18_after_startup() {
    AddonManager.getAddonByID("bootstrap1@tests.mozilla.org", function(b1) {
      
      do_check_eq(getInstalledVersion(), 2);
      do_check_eq(getActiveVersion(), 2);
      do_check_neq(b1, null);
      do_check_eq(b1.version, "2.0");
      do_check_true(b1.isActive);

      do_check_eq(getShutdownReason(), ADDON_UPGRADE);
      do_check_eq(getUninstallReason(), ADDON_UPGRADE);
      do_check_eq(getInstallReason(), ADDON_UPGRADE);
      do_check_eq(getStartupReason(), ADDON_UPGRADE);

      do_check_eq(getShutdownNewVersion(), 2);
      do_check_eq(getUninstallNewVersion(), 2);
      do_check_eq(getInstallOldVersion(), 1);
      do_check_eq(getStartupOldVersion(), 1);

      do_check_bootstrappedPref(run_test_19);
    });
  });
  installAllFiles([do_get_addon("test_bootstrap1_2")], function() { });
}


function run_test_19() {
  resetPrefs();
  AddonManager.getAddonByID("bootstrap1@tests.mozilla.org", function(b1) {
    
    prepare_test({
      "bootstrap1@tests.mozilla.org": [
        ["onUninstalling", false],
        "onUninstalled",
        ["onInstalling", false],
        "onInstalled"
      ]
    }, [], check_test_19);

    b1.uninstall();
  });
}

function check_test_19() {
  AddonManager.getAddonByID("bootstrap1@tests.mozilla.org", function(b1) {
    
    do_check_eq(getInstalledVersion(), 1);
    do_check_eq(getActiveVersion(), 1);
    do_check_neq(b1, null);
    do_check_eq(b1.version, "1.0");
    do_check_true(b1.isActive);

    
    do_check_eq(getShutdownReason(), ADDON_UNINSTALL);
    do_check_eq(getUninstallReason(), ADDON_UNINSTALL);
    do_check_eq(getInstallReason(), ADDON_INSTALL);
    do_check_eq(getStartupReason(), ADDON_INSTALL);

    do_check_eq(getShutdownNewVersion(), 0);
    do_check_eq(getUninstallNewVersion(), 0);
    do_check_eq(getInstallOldVersion(), 0);
    do_check_eq(getStartupOldVersion(), 0);

    do_check_bootstrappedPref(run_test_20);
  });
}



function run_test_20() {
  resetPrefs();
  shutdownManager();

  manuallyInstall(do_get_addon("test_bootstrap1_2"), profileDir,
                  "bootstrap1@tests.mozilla.org");

  startupManager();

  AddonManager.getAddonByID("bootstrap1@tests.mozilla.org", function(b1) {
    
    do_check_eq(getInstalledVersion(), 2);
    do_check_eq(getActiveVersion(), 2);
    do_check_neq(b1, null);
    do_check_eq(b1.version, "2.0");
    do_check_true(b1.isActive);

    do_check_eq(getShutdownReason(), APP_SHUTDOWN);
    do_check_eq(getUninstallReason(), ADDON_UPGRADE);
    do_check_eq(getInstallReason(), ADDON_UPGRADE);
    do_check_eq(getStartupReason(), APP_STARTUP);

    do_check_eq(getShutdownNewVersion(), 0);
    do_check_eq(getUninstallNewVersion(), 2);
    do_check_eq(getInstallOldVersion(), 1);
    do_check_eq(getStartupOldVersion(), 0);

    do_execute_soon(run_test_21);
  });
}


function run_test_21() {
  resetPrefs();
  shutdownManager();

  manuallyUninstall(profileDir, "bootstrap1@tests.mozilla.org");

  startupManager();

  AddonManager.getAddonByID("bootstrap1@tests.mozilla.org", function(b1) {
    
    do_check_eq(getInstalledVersion(), 1);
    do_check_eq(getActiveVersion(), 1);
    do_check_neq(b1, null);
    do_check_eq(b1.version, "1.0");
    do_check_true(b1.isActive);

    do_check_eq(getShutdownReason(), APP_SHUTDOWN);
    do_check_eq(getShutdownNewVersion(), 0);

    
    
    do_check_eq(getUninstallReason(), -1);
    do_check_eq(getUninstallNewVersion(), -1);

    
    do_check_eq(getInstallReason(), ADDON_INSTALL);
    do_check_eq(getInstallOldVersion(), 0);

    do_check_eq(getStartupReason(), APP_STARTUP);
    do_check_eq(getStartupOldVersion(), 0);

    do_check_bootstrappedPref(function() {
      manuallyUninstall(userExtDir, "bootstrap1@tests.mozilla.org");

      restartManager();
      run_test_22();
    });
  });
}


function run_test_22() {
  shutdownManager();

  let file = manuallyInstall(do_get_addon("test_bootstrap1_1"), profileDir,
                             "bootstrap1@tests.mozilla.org");

  
  setExtensionModifiedTime(file, file.lastModifiedTime - 5000);

  startupManager();

  AddonManager.getAddonByID("bootstrap1@tests.mozilla.org", callback_soon(function(b1) {
    
    do_check_eq(getInstalledVersion(), 1);
    do_check_eq(getActiveVersion(), 1);
    do_check_neq(b1, null);
    do_check_eq(b1.version, "1.0");
    do_check_true(b1.isActive);

    resetPrefs();
    shutdownManager();

    manuallyUninstall(profileDir, "bootstrap1@tests.mozilla.org");
    manuallyInstall(do_get_addon("test_bootstrap1_2"), profileDir,
                    "bootstrap1@tests.mozilla.org");

    startupManager();

    AddonManager.getAddonByID("bootstrap1@tests.mozilla.org", function(b1) {
      
      do_check_eq(getInstalledVersion(), 2);
      do_check_eq(getActiveVersion(), 2);
      do_check_neq(b1, null);
      do_check_eq(b1.version, "2.0");
      do_check_true(b1.isActive);

      do_check_eq(getShutdownReason(), APP_SHUTDOWN);
      do_check_eq(getShutdownNewVersion(), 0);

      
      
      do_check_eq(getUninstallReason(), -1);
      do_check_eq(getUninstallNewVersion(), -1);

      do_check_eq(getInstallReason(), ADDON_UPGRADE);
      do_check_eq(getInstallOldVersion(), 1);
      do_check_eq(getStartupReason(), APP_STARTUP);
      do_check_eq(getStartupOldVersion(), 0);

      do_check_bootstrappedPref(function() {
        b1.uninstall();

        run_test_23();
      });
    });
  }));
}



function run_test_23() {
  prepare_test({ }, [
    "onNewInstall"
  ]);

  let url = "http://localhost:" + gPort + "/addons/test_bootstrap1_1.xpi";
  AddonManager.getInstallForURL(url, function(install) {
    ensure_test_completed();

    do_check_neq(install, null);

    prepare_test({ }, [
      "onDownloadStarted",
      "onDownloadEnded"
    ], function() {
      do_check_eq(install.type, "extension");
      do_check_eq(install.version, "1.0");
      do_check_eq(install.name, "Test Bootstrap 1");
      do_check_eq(install.state, AddonManager.STATE_DOWNLOADED);
      do_check_true(install.addon.hasResource("install.rdf"));
      do_check_true(install.addon.hasResource("bootstrap.js"));
      do_check_false(install.addon.hasResource("foo.bar"));
      do_check_eq(install.addon.operationsRequiringRestart &
                  AddonManager.OP_NEEDS_RESTART_INSTALL, 0);
      do_check_not_in_crash_annotation("bootstrap1@tests.mozilla.org", "1.0");

      let addon = install.addon;
      prepare_test({
        "bootstrap1@tests.mozilla.org": [
          ["onInstalling", false],
          "onInstalled"
        ]
      }, [
        "onInstallStarted",
        "onInstallEnded",
      ], function() {
        do_check_true(addon.hasResource("install.rdf"));
        do_check_bootstrappedPref(check_test_23);
      });
    });
    install.install();
  }, "application/x-xpinstall");
}

function check_test_23() {
  AddonManager.getAllInstalls(function(installs) {
    
    
    do_check_eq(installs.length, 0);

    AddonManager.getAddonByID("bootstrap1@tests.mozilla.org", function(b1) {
     do_execute_soon(function test_23_after_startup() {
      do_check_neq(b1, null);
      do_check_eq(b1.version, "1.0");
      do_check_false(b1.appDisabled);
      do_check_false(b1.userDisabled);
      do_check_true(b1.isActive);
      do_check_eq(getInstalledVersion(), 1);
      do_check_eq(getActiveVersion(), 1);
      do_check_eq(getStartupReason(), ADDON_INSTALL);
      do_check_eq(getStartupOldVersion(), 0);
      do_check_true(b1.hasResource("install.rdf"));
      do_check_true(b1.hasResource("bootstrap.js"));
      do_check_false(b1.hasResource("foo.bar"));
      do_check_in_crash_annotation("bootstrap1@tests.mozilla.org", "1.0");

      let dir = do_get_addon_root_uri(profileDir, "bootstrap1@tests.mozilla.org");
      do_check_eq(b1.getResourceURI("bootstrap.js").spec, dir + "bootstrap.js");

      AddonManager.getAddonsWithOperationsByTypes(null, callback_soon(function(list) {
        do_check_eq(list.length, 0);

        restartManager();
        AddonManager.getAddonByID("bootstrap1@tests.mozilla.org", callback_soon(function(b1) {
          b1.uninstall();
          restartManager();

          testserver.stop(run_test_24);
        }));
      }));
     });
    });
  });
}


function run_test_24() {
  resetPrefs();
  do_print("starting 24");

  Promise.all([promisePref("bootstraptest2.active_version"),
              promiseInstall([do_get_addon("test_bootstrap1_1"), do_get_addon("test_bootstrap2_1")])])
         .then(function test_24_pref() {
    do_print("test 24 got prefs");
    do_check_eq(getInstalledVersion(), 1);
    do_check_eq(getActiveVersion(), 1);
    do_check_eq(getInstalledVersion2(), 1);
    do_check_eq(getActiveVersion2(), 1);

    resetPrefs();

    restartManager();

    do_check_eq(getInstalledVersion(), -1);
    do_check_eq(getActiveVersion(), 1);
    do_check_eq(getInstalledVersion2(), -1);
    do_check_eq(getActiveVersion2(), 1);

    shutdownManager();

    do_check_eq(getInstalledVersion(), -1);
    do_check_eq(getActiveVersion(), 0);
    do_check_eq(getInstalledVersion2(), -1);
    do_check_eq(getActiveVersion2(), 0);

    
    let bootstrappedAddons = JSON.parse(Services.prefs.getCharPref("extensions.bootstrappedAddons"));
    bootstrappedAddons["bootstrap1@tests.mozilla.org"].descriptor += "foo";
    Services.prefs.setCharPref("extensions.bootstrappedAddons", JSON.stringify(bootstrappedAddons));

    startupManager(false);

    do_check_eq(getInstalledVersion(), -1);
    do_check_eq(getActiveVersion(), 1);
    do_check_eq(getInstalledVersion2(), -1);
    do_check_eq(getActiveVersion2(), 1);

    run_test_25();
  });
}



function run_test_25() {
  waitForPref("bootstraptest.startup_reason", function test_25_after_pref() {
      do_print("test 25 pref change detected");
      do_check_eq(getInstalledVersion(), 1);
      do_check_eq(getActiveVersion(), 1);

      installAllFiles([do_get_addon("test_bootstrap1_4")], function() {
        
        do_check_eq(getInstalledVersion(), 1);
        do_check_eq(getActiveVersion(), 1);

        AddonManager.getAddonByID("bootstrap1@tests.mozilla.org", callback_soon(function(b1) {
          do_check_neq(b1, null);
          do_check_eq(b1.version, "1.0");
          do_check_true(b1.isActive);
          do_check_true(hasFlag(b1.pendingOperations, AddonManager.PENDING_UPGRADE));

          restartManager();

          do_check_eq(getInstalledVersion(), 0);
          do_check_eq(getUninstallReason(), ADDON_UPGRADE);
          do_check_eq(getUninstallNewVersion(), 4);
          do_check_eq(getActiveVersion(), 0);

          AddonManager.getAddonByID("bootstrap1@tests.mozilla.org", function(b1) {
            do_check_neq(b1, null);
            do_check_eq(b1.version, "4.0");
            do_check_true(b1.isActive);
            do_check_eq(b1.pendingOperations, AddonManager.PENDING_NONE);

            do_check_bootstrappedPref(run_test_26);
          });
        }));
      });
  });
  installAllFiles([do_get_addon("test_bootstrap1_1")], function test_25_installed() {
    do_print("test 25 install done");
  });
}



function run_test_26() {
  installAllFiles([do_get_addon("test_bootstrap1_1")], function() {
    
    do_check_eq(getInstalledVersion(), 0);
    do_check_eq(getActiveVersion(), 0);

    AddonManager.getAddonByID("bootstrap1@tests.mozilla.org", callback_soon(function(b1) {
      do_check_neq(b1, null);
      do_check_eq(b1.version, "4.0");
      do_check_true(b1.isActive);
      do_check_true(hasFlag(b1.pendingOperations, AddonManager.PENDING_UPGRADE));

      restartManager();

      do_check_eq(getInstalledVersion(), 1);
      do_check_eq(getInstallReason(), ADDON_DOWNGRADE);
      do_check_eq(getInstallOldVersion(), 4);
      do_check_eq(getActiveVersion(), 1);

      AddonManager.getAddonByID("bootstrap1@tests.mozilla.org", function(b1) {
        do_check_neq(b1, null);
        do_check_eq(b1.version, "1.0");
        do_check_true(b1.isActive);
        do_check_eq(b1.pendingOperations, AddonManager.PENDING_NONE);

        do_check_bootstrappedPref(run_test_27);
      });
    }));
  });
}



function run_test_27() {
  AddonManager.getAddonByID("bootstrap1@tests.mozilla.org", function(b1) {
    do_check_neq(b1, null);
    b1.userDisabled = true;
    do_check_eq(b1.version, "1.0");
    do_check_false(b1.isActive);
    do_check_eq(b1.pendingOperations, AddonManager.PENDING_NONE);
    do_check_eq(getInstalledVersion(), 1);
    do_check_eq(getActiveVersion(), 0);

    installAllFiles([do_get_addon("test_bootstrap1_4")], function() {
      
      do_check_eq(getInstalledVersion(), 0);
      do_check_eq(getUninstallReason(), ADDON_UPGRADE);
      do_check_eq(getUninstallNewVersion(), 4);
      do_check_eq(getActiveVersion(), 0);

      AddonManager.getAddonByID("bootstrap1@tests.mozilla.org", callback_soon(function(b1) {
        do_check_neq(b1, null);
        do_check_eq(b1.version, "4.0");
        do_check_false(b1.isActive);
        do_check_eq(b1.pendingOperations, AddonManager.PENDING_NONE);

        restartManager();

        do_check_eq(getInstalledVersion(), 0);
        do_check_eq(getActiveVersion(), 0);

        AddonManager.getAddonByID("bootstrap1@tests.mozilla.org", function(b1) {
          do_check_neq(b1, null);
          do_check_eq(b1.version, "4.0");
          do_check_false(b1.isActive);
          do_check_eq(b1.pendingOperations, AddonManager.PENDING_NONE);

          do_check_bootstrappedPref(run_test_28);
        });
      }));
    });
  });
}



function run_test_28() {
  installAllFiles([do_get_addon("test_bootstrap1_1")], function() {
   do_execute_soon(function bootstrap_disabled_downgrade_check() {
    
    do_check_eq(getInstalledVersion(), 1);
    do_check_eq(getInstallReason(), ADDON_DOWNGRADE);
    do_check_eq(getInstallOldVersion(), 4);
    do_check_eq(getActiveVersion(), 0);

    AddonManager.getAddonByID("bootstrap1@tests.mozilla.org", callback_soon(function(b1) {
      do_check_neq(b1, null);
      do_check_eq(b1.version, "1.0");
      do_check_false(b1.isActive);
      do_check_true(b1.userDisabled);
      do_check_eq(b1.pendingOperations, AddonManager.PENDING_NONE);

      restartManager();

      do_check_eq(getInstalledVersion(), 1);
      do_check_eq(getActiveVersion(), 0);

      AddonManager.getAddonByID("bootstrap1@tests.mozilla.org", function(b1) {
        do_check_neq(b1, null);
        do_check_true(b1.userDisabled);
        b1.userDisabled = false;
        do_check_eq(b1.version, "1.0");
        do_check_true(b1.isActive);
        do_check_eq(b1.pendingOperations, AddonManager.PENDING_NONE);
        do_check_eq(getInstalledVersion(), 1);
        do_check_eq(getActiveVersion(), 1);

        do_check_bootstrappedPref(do_test_finished);
      });
    }));
   });
  });
}
