






Services.prefs.setBoolPref("extensions.checkUpdateSecurity", false);

Services.prefs.setBoolPref("extensions.hotfix.cert.checkAttributes", false);

Components.utils.import("resource://testing-common/httpd.js");
var testserver;
const profileDir = gProfD.clone();
profileDir.append("extensions");

function run_test() {
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "1.9.2");

  
  testserver = new HttpServer();
  testserver.registerDirectory("/data/", do_get_file("data"));
  testserver.registerDirectory("/addons/", do_get_file("addons"));
  testserver.start(4444);

  startupManager();

  do_test_pending();
  run_test_1();
}

function end_test() {
  testserver.stop(do_test_finished);
}


function run_test_1() {
  Services.prefs.setCharPref("extensions.hotfix.id", "hotfix@tests.mozilla.org");
  Services.prefs.setCharPref("extensions.update.background.url", "http://localhost:4444/data/test_hotfix_1.rdf");

  prepare_test({
    "hotfix@tests.mozilla.org": [
      "onInstalling"
    ]
  }, [
    "onNewInstall",
    "onDownloadStarted",
    "onDownloadEnded",
    "onInstallStarted",
    "onInstallEnded",
  ], check_test_1);

  
  gInternalManager.notify(null);
}

function check_test_1() {
  restartManager();

  AddonManager.getAddonByID("hotfix@tests.mozilla.org", function(aAddon) {
    do_check_neq(aAddon, null);
    do_check_eq(aAddon.version, "1.0");

    aAddon.uninstall();
    do_execute_soon(run_test_2);
  });
}


function run_test_2() {
  restartManager();

  AddonManager.addInstallListener({
    onNewInstall: function() {
      do_throw("Should not have seen a new install created");
    }
  });

  function observer() {
    Services.obs.removeObserver(arguments.callee, "addons-background-update-complete");
    do_execute_soon(run_test_3);
  }

  Services.obs.addObserver(observer, "addons-background-update-complete", false);

  
  gInternalManager.notify(null);
}


function run_test_3() {
  restartManager();
  Services.prefs.setCharPref("extensions.hotfix.url", "http://localhost:4444/data/test_hotfix_2.rdf");

  prepare_test({
    "hotfix@tests.mozilla.org": [
      "onInstalling"
    ]
  }, [
    "onNewInstall",
    "onDownloadStarted",
    "onDownloadEnded",
    "onInstallStarted",
    "onInstallEnded",
  ], check_test_3);

  
  gInternalManager.notify(null);
}

function check_test_3() {
  restartManager();

  AddonManager.getAddonByID("hotfix@tests.mozilla.org", function(aAddon) {
    do_check_neq(aAddon, null);
    do_check_eq(aAddon.version, "2.0");

    aAddon.uninstall();
    do_execute_soon(run_test_4);
  });
}


function run_test_4() {
  restartManager();

  Services.prefs.setCharPref("extensions.hotfix.url", "http://localhost:4444/data/test_hotfix_3.rdf");

  AddonManager.addInstallListener({
    onNewInstall: function() {
      do_throw("Should not have seen a new install created");
    }
  });

  function observer() {
    Services.obs.removeObserver(arguments.callee, "addons-background-update-complete");
    do_execute_soon(run_test_5);
  }

  Services.obs.addObserver(observer, "addons-background-update-complete", false);

  
  gInternalManager.notify(null);
}


function run_test_5() {
  restartManager();

  Services.prefs.setCharPref("extensions.hotfix.url", "http://localhost:4444/data/test_hotfix_1.rdf");

  AddonManager.addInstallListener({
    onNewInstall: function() {
      do_throw("Should not have seen a new install created");
    }
  });

  function observer() {
    Services.obs.removeObserver(arguments.callee, "addons-background-update-complete");
    do_execute_soon(run_test_6);
  }

  Services.obs.addObserver(observer, "addons-background-update-complete", false);

  
  gInternalManager.notify(null);
}


function run_test_6() {
  restartManager();

  Services.prefs.setCharPref("extensions.hotfix.lastVersion", "0");
  Services.prefs.setCharPref("extensions.hotfix.url", "http://localhost:4444/data/test_hotfix_1.rdf");

  prepare_test({
    "hotfix@tests.mozilla.org": [
      "onInstalling"
    ]
  }, [
    "onNewInstall",
    "onDownloadStarted",
    "onDownloadEnded",
    "onInstallStarted",
    "onInstallEnded",
  ], check_test_6);

  
  gInternalManager.notify(null);
}

function check_test_6() {
  AddonManager.addInstallListener({
    onNewInstall: function() {
      do_throw("Should not have seen a new install created");
    }
  });

  function observer() {
    Services.obs.removeObserver(arguments.callee, "addons-background-update-complete");
    restartManager();

    AddonManager.getAddonByID("hotfix@tests.mozilla.org", function(aAddon) {
      aAddon.uninstall();
      do_execute_soon(run_test_7);
    });
  }

  Services.obs.addObserver(observer, "addons-background-update-complete", false);

  
  gInternalManager.notify(null);
}


function run_test_7() {
  restartManager();

  Services.prefs.setCharPref("extensions.hotfix.lastVersion", "0");

  prepare_test({
    "hotfix@tests.mozilla.org": [
      "onInstalling"
    ]
  }, [
    "onNewInstall",
    "onDownloadStarted",
    "onDownloadEnded",
    "onInstallStarted",
    "onInstallEnded",
  ], check_test_7);

  
  gInternalManager.notify(null);
}

function check_test_7(aInstall) {
  prepare_test({
    "hotfix@tests.mozilla.org": [
      "onOperationCancelled"
    ]
  }, [
    "onInstallCancelled",
  ]);

  aInstall.cancel();

  prepare_test({
    "hotfix@tests.mozilla.org": [
      "onInstalling"
    ]
  }, [
    "onNewInstall",
    "onDownloadStarted",
    "onDownloadEnded",
    "onInstallStarted",
    "onInstallEnded",
  ], finish_test_7);

  
  gInternalManager.notify(null);
}

function finish_test_7() {
  restartManager();

  AddonManager.getAddonByID("hotfix@tests.mozilla.org", function(aAddon) {
    do_check_neq(aAddon, null);
    do_check_eq(aAddon.version, "1.0");

    aAddon.uninstall();
    do_execute_soon(run_test_8);
  });
}


function run_test_8() {
  restartManager();

  Services.prefs.setCharPref("extensions.hotfix.lastVersion", "0");
  Services.prefs.setCharPref("extensions.hotfix.url", "http://localhost:4444/data/test_hotfix_1.rdf");

  prepare_test({
    "hotfix@tests.mozilla.org": [
      "onInstalling"
    ]
  }, [
    "onNewInstall",
    "onDownloadStarted",
    "onDownloadEnded",
    "onInstallStarted",
    "onInstallEnded",
  ], check_test_8);

  
  gInternalManager.notify(null);
}

function check_test_8() {
  Services.prefs.setCharPref("extensions.hotfix.url", "http://localhost:4444/data/test_hotfix_2.rdf");

  prepare_test({
    "hotfix@tests.mozilla.org": [
      "onOperationCancelled",
      "onInstalling"
    ]
  }, [
    "onNewInstall",
    "onDownloadStarted",
    "onDownloadEnded",
    "onInstallStarted",
    "onInstallCancelled",
    "onInstallEnded",
  ], finish_test_8);

  
  gInternalManager.notify(null);
}

function finish_test_8() {
  AddonManager.getAllInstalls(function(aInstalls) {
    do_check_eq(aInstalls.length, 1);
    do_check_eq(aInstalls[0].version, "2.0");

    restartManager();

    AddonManager.getAddonByID("hotfix@tests.mozilla.org", function(aAddon) {
      do_check_neq(aAddon, null);
      do_check_eq(aAddon.version, "2.0");

      aAddon.uninstall();
      restartManager();

      end_test();
    });
  });
}
