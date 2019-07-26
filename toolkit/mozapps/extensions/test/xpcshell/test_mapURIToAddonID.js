





Components.utils.import("resource://gre/modules/Services.jsm");


Services.prefs.setIntPref("extensions.enabledScopes",
                          AddonManager.SCOPE_PROFILE + AddonManager.SCOPE_USER);

createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "1.9.2");

const profileDir = gProfD.clone();
profileDir.append("extensions");
const userExtDir = gProfD.clone();
userExtDir.append("extensions2");
userExtDir.append(gAppInfo.ID);
registerDirectory("XREUSysExt", userExtDir.parent);

function TestProvider(result) {
  this.result = result;
}
TestProvider.prototype = {
  uri: Services.io.newURI("hellow://world", null, null),
  id: "valid@id",
  startup: function() {},
  shutdown: function() {},
  mapURIToAddonID: function(aURI) {
    if (aURI.spec === this.uri.spec) {
      return this.id;
    }
    throw Components.Exception("Not mapped", result);
  }
};

function TestProviderNoMap() {}
TestProviderNoMap.prototype = {
  startup: function() {},
  shutdown: function() {}
};

function check_mapping(uri, id) {
  do_check_eq(AddonManager.mapURIToAddonID(uri), id);
  let svc = Components.classes["@mozilla.org/addons/integration;1"].
            getService(Components.interfaces.amIAddonManager);
  let val = {};
  do_check_true(svc.mapURIToAddonID(uri, val));
  do_check_eq(val.value, id);
}

function resetPrefs() {
  Services.prefs.setIntPref("bootstraptest.active_version", -1);
}

function waitForPref(aPref, aCallback) {
  function prefChanged() {
    Services.prefs.removeObserver(aPref, prefChanged);
    aCallback();
  }
  Services.prefs.addObserver(aPref, prefChanged, false);
}

function getActiveVersion() {
  return Services.prefs.getIntPref("bootstraptest.active_version");
}

function run_test() {
  do_test_pending();

  resetPrefs();

  run_test_early();
}

function run_test_early() {
  startupManager();

  installAllFiles([do_get_addon("test_chromemanifest_1")], function() {
    restartManager();
    AddonManager.getAddonByID("addon1@tests.mozilla.org", function(addon) {
      let uri = addon.getResourceURI(".");
      let id = addon.id;
      check_mapping(uri, id);

      shutdownManager();

      
      
      

      
      let s = Components.utils.import(
        "resource://gre/modules/addons/XPIProvider.jsm", {});

      
      do_check_null(s.XPIProvider.mapURIToAddonID(uri));
      do_check_null(AddonManager.mapURIToAddonID(uri));

      
      startupManager(false);

      
      check_mapping(uri, id);
      do_check_eq(s.XPIProvider.mapURIToAddonID(uri), id);

      run_test_nomapping();
    });
  });
}

function run_test_nomapping() {
  do_check_eq(AddonManager.mapURIToAddonID(TestProvider.prototype.uri), null);
  try {
    let svc = Components.classes["@mozilla.org/addons/integration;1"].
              getService(Components.interfaces.amIAddonManager);
    let val = {};
    do_check_false(svc.mapURIToAddonID(TestProvider.prototype.uri, val));
  }
  catch (ex) {
    do_throw(ex);
  }

  run_test_1();
}



function run_test_1() {
  prepare_test({ }, [
    "onNewInstall"
  ]);

  AddonManager.getInstallForFile(do_get_addon("test_bootstrap1_1"), function(install) {
    ensure_test_completed();

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
      let uri = addon.getResourceURI(".");
      check_mapping(uri, addon.id);

      waitForPref("bootstraptest.active_version", function() {
        run_test_2(uri);
      });
    });
    install.install();
  });
}



function run_test_2(uri) {
  AddonManager.getAddonByID("bootstrap1@tests.mozilla.org", function(b1) {
    prepare_test({
      "bootstrap1@tests.mozilla.org": [
        ["onDisabling", false],
        "onDisabled"
      ]
    });

    b1.userDisabled = true;
    ensure_test_completed();

    AddonManager.getAddonByID("bootstrap1@tests.mozilla.org", function(newb1) {
      do_check_true(newb1.userDisabled);
      check_mapping(uri, newb1.id);

      do_execute_soon(() => run_test_3(uri));
    });
  });
}



function run_test_3(uri) {
  restartManager();

  check_mapping(uri, "bootstrap1@tests.mozilla.org");

  run_test_4();
}


function run_test_4() {
  restartManager();

  AddonManager.getAddonByID("bootstrap1@tests.mozilla.org", function(b1) {
    prepare_test({
      "bootstrap1@tests.mozilla.org": [
        ["onEnabling", false],
        "onEnabled"
      ]
    });

    b1.userDisabled = false;
    ensure_test_completed();

    AddonManager.getAddonByID("bootstrap1@tests.mozilla.org", function(newb1) {
      let uri = newb1.getResourceURI(".");
      check_mapping(uri, newb1.id);

      do_execute_soon(run_test_5);
    });
  });
}


function run_test_5() {
  restartManager();

  AddonManager.getAddonByID("bootstrap1@tests.mozilla.org", function(b1) {
    let uri = b1.getResourceURI(".");
    check_mapping(uri, b1.id);

    do_execute_soon(run_test_6);
  });
}


function run_test_6() {
  AddonManager.getAddonByID("bootstrap1@tests.mozilla.org", function(b1) {
    prepare_test({
      "bootstrap1@tests.mozilla.org": [
        ["onUninstalling", false],
        "onUninstalled"
      ]
    });

    let uri = b1.getResourceURI(".");
    b1.uninstall();
    ensure_test_completed();

    check_mapping(uri, b1.id);

    restartManager();
    do_execute_soon(run_test_7);
  });
}


function run_test_7() {
  shutdownManager();

  manuallyInstall(do_get_addon("test_bootstrap1_1"), profileDir, "bootstrap1@tests.mozilla.org");

  startupManager();

  AddonManager.getAddonByID("bootstrap1@tests.mozilla.org", function(b1) {
    let uri = b1.getResourceURI(".");
    check_mapping(uri, b1.id);

    do_execute_soon(run_test_invalidarg);
  });
}



function run_test_invalidarg() {
  restartManager();

  let tests = [undefined,
               null,
               1,
               "string",
               "chrome://global/content/",
               function() {}
               ];
  for (var test of tests) {
    try {
      AddonManager.mapURIToAddonID(test);
      throw new Error("Shouldn't be able to map the URI in question");
    }
    catch (ex if ex.result) {
      do_check_eq(ex.result, Components.results.NS_ERROR_INVALID_ARG);
    }
    catch (ex) {
      do_throw(ex);
    }
  }

  run_test_provider();
}


function run_test_provider() {
  restartManager();

  const provider = new TestProvider(Components.results.NS_ERROR_NOT_AVAILABLE);
  AddonManagerPrivate.registerProvider(provider);

  check_mapping(provider.uri, provider.id);

  let u2 = provider.uri.clone();
  u2.path = "notmapped";
  do_check_eq(AddonManager.mapURIToAddonID(u2), null);

  AddonManagerPrivate.unregisterProvider(provider);

  run_test_provider_nomap();
}



function run_test_provider_nomap() {
  restartManager();

  const provider = new TestProviderNoMap();
  AddonManagerPrivate.registerProvider(provider);

  do_check_eq(AddonManager.mapURIToAddonID(TestProvider.prototype.uri), null);

  AddonManagerPrivate.unregisterProvider(provider);

  do_test_finished();
}


