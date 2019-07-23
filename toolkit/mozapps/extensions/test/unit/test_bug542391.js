






































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

const URI_EXTENSION_UPDATE_DIALOG     = "chrome://mozapps/content/extensions/update.xul";
const PREF_EM_DISABLED_ADDONS_LIST    = "extensions.disabledAddons";
const PREF_EM_SHOW_MISMATCH_UI        = "extensions.showMismatchUI";



var WindowWatcher = {
  expected: false,
  arguments: null,

  openWindow: function(parent, url, name, features, arguments) {
    do_check_eq(url, URI_EXTENSION_UPDATE_DIALOG);
    do_check_true(this.expected);
    this.expected = false;
    this.arguments = arguments.QueryInterface(Ci.nsIVariant);
  },

  QueryInterface: function(iid) {
    if (iid.equals(Ci.nsIWindowWatcher)
     || iid.equals(Ci.nsISupports))
      return this;

    throw Cr.NS_ERROR_NO_INTERFACE;
  }
}

var WindowWatcherFactory = {
  createInstance: function createInstance(outer, iid) {
    if (outer != null)
      throw Components.results.NS_ERROR_NO_AGGREGATION;
    return WindowWatcher.QueryInterface(iid);
  }
};

var registrar = Components.manager.QueryInterface(Components.interfaces.nsIComponentRegistrar);
registrar.registerFactory(Components.ID("{1dfeb90a-2193-45d5-9cb8-864928b2af55}"),
                          "Fake Window Watcher",
                          "@mozilla.org/embedcomp/window-watcher;1", WindowWatcherFactory);

function isDisabled(id) {
  return getManifestProperty(id, "isDisabled") == "true";
}

function appDisabled(id) {
  return getManifestProperty(id, "appDisabled") == "true";
}

function userDisabled(id) {
  return getManifestProperty(id, "userDisabled") == "true";
}

function check_state_v1() {
  do_check_neq(gEM.getItemForID("bug542391_1@tests.mozilla.org"), null);
  do_check_false(appDisabled("bug542391_1@tests.mozilla.org"));
  do_check_false(userDisabled("bug542391_1@tests.mozilla.org"));

  do_check_neq(gEM.getItemForID("bug542391_2@tests.mozilla.org"), null);
  do_check_false(appDisabled("bug542391_2@tests.mozilla.org"));
  do_check_true(userDisabled("bug542391_2@tests.mozilla.org"));

  do_check_neq(gEM.getItemForID("bug542391_3@tests.mozilla.org"), null);
  do_check_false(appDisabled("bug542391_3@tests.mozilla.org"));
  do_check_false(userDisabled("bug542391_3@tests.mozilla.org"));

  do_check_neq(gEM.getItemForID("bug542391_4@tests.mozilla.org"), null);
  do_check_false(appDisabled("bug542391_4@tests.mozilla.org"));
  do_check_true(userDisabled("bug542391_4@tests.mozilla.org"));

  do_check_neq(gEM.getItemForID("bug542391_5@tests.mozilla.org"), null);
  do_check_false(appDisabled("bug542391_5@tests.mozilla.org"));
  do_check_false(userDisabled("bug542391_5@tests.mozilla.org"));
}

function check_state_v2() {
  do_check_neq(gEM.getItemForID("bug542391_1@tests.mozilla.org"), null);
  do_check_true(appDisabled("bug542391_1@tests.mozilla.org"));
  do_check_false(userDisabled("bug542391_1@tests.mozilla.org"));

  do_check_neq(gEM.getItemForID("bug542391_2@tests.mozilla.org"), null);
  do_check_false(appDisabled("bug542391_2@tests.mozilla.org"));
  do_check_true(userDisabled("bug542391_2@tests.mozilla.org"));

  do_check_neq(gEM.getItemForID("bug542391_3@tests.mozilla.org"), null);
  do_check_false(appDisabled("bug542391_3@tests.mozilla.org"));
  do_check_false(userDisabled("bug542391_3@tests.mozilla.org"));

  do_check_neq(gEM.getItemForID("bug542391_4@tests.mozilla.org"), null);
  do_check_false(appDisabled("bug542391_4@tests.mozilla.org"));
  do_check_true(userDisabled("bug542391_4@tests.mozilla.org"));

  do_check_neq(gEM.getItemForID("bug542391_5@tests.mozilla.org"), null);
  do_check_false(appDisabled("bug542391_5@tests.mozilla.org"));
  do_check_false(userDisabled("bug542391_5@tests.mozilla.org"));
}

function check_state_v3() {
  do_check_neq(gEM.getItemForID("bug542391_1@tests.mozilla.org"), null);
  do_check_true(appDisabled("bug542391_1@tests.mozilla.org"));
  do_check_false(userDisabled("bug542391_1@tests.mozilla.org"));

  do_check_neq(gEM.getItemForID("bug542391_2@tests.mozilla.org"), null);
  do_check_true(appDisabled("bug542391_2@tests.mozilla.org"));
  do_check_true(userDisabled("bug542391_2@tests.mozilla.org"));

  do_check_neq(gEM.getItemForID("bug542391_3@tests.mozilla.org"), null);
  do_check_true(appDisabled("bug542391_3@tests.mozilla.org"));
  do_check_false(userDisabled("bug542391_3@tests.mozilla.org"));

  do_check_neq(gEM.getItemForID("bug542391_4@tests.mozilla.org"), null);
  do_check_false(appDisabled("bug542391_4@tests.mozilla.org"));
  do_check_true(userDisabled("bug542391_4@tests.mozilla.org"));

  do_check_neq(gEM.getItemForID("bug542391_5@tests.mozilla.org"), null);
  do_check_false(appDisabled("bug542391_5@tests.mozilla.org"));
  do_check_false(userDisabled("bug542391_5@tests.mozilla.org"));
}



function run_test() {
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "1");

  startupEM();

  gEM.installItemFromFile(do_get_addon("test_bug542391_1"),
                          NS_INSTALL_LOCATION_APPPROFILE);
  gEM.installItemFromFile(do_get_addon("test_bug542391_2"),
                          NS_INSTALL_LOCATION_APPPROFILE);
  gEM.installItemFromFile(do_get_addon("test_bug542391_3"),
                          NS_INSTALL_LOCATION_APPPROFILE);
  gEM.installItemFromFile(do_get_addon("test_bug542391_4"),
                          NS_INSTALL_LOCATION_APPPROFILE);
  gEM.installItemFromFile(do_get_addon("test_bug542391_5"),
                          NS_INSTALL_LOCATION_APPPROFILE);

  restartEM();
  gEM.disableItem("bug542391_2@tests.mozilla.org");
  gEM.disableItem("bug542391_4@tests.mozilla.org");
  restartEM();

  check_state_v1();

  WindowWatcher.expected = true;
  restartEM("2");
  do_check_false(WindowWatcher.expected);
  check_state_v2();

  run_test_1();
}



function run_test_1() {
  WindowWatcher.expected = true;
  restartEM("3");
  do_check_false(WindowWatcher.expected);
  check_state_v3();
  do_check_eq(WindowWatcher.arguments.length, 3);
  do_check_true(WindowWatcher.arguments.indexOf("bug542391_1@tests.mozilla.org") >= 0);
  do_check_true(WindowWatcher.arguments.indexOf("bug542391_2@tests.mozilla.org") >= 0);
  do_check_true(WindowWatcher.arguments.indexOf("bug542391_4@tests.mozilla.org") >= 0);

  run_test_2();
}



function run_test_2() {
  WindowWatcher.expected = true;
  restartEM("2");
  do_check_false(WindowWatcher.expected);
  check_state_v2();
  do_check_eq(WindowWatcher.arguments.length, 4);
  do_check_true(WindowWatcher.arguments.indexOf("bug542391_1@tests.mozilla.org") >= 0);
  do_check_true(WindowWatcher.arguments.indexOf("bug542391_2@tests.mozilla.org") >= 0);
  do_check_true(WindowWatcher.arguments.indexOf("bug542391_3@tests.mozilla.org") >= 0);
  do_check_true(WindowWatcher.arguments.indexOf("bug542391_4@tests.mozilla.org") >= 0);

  run_test_4();
}


function run_test_3() {
  gPrefs.setBoolPref(PREF_EM_SHOW_MISMATCH_UI, false);

  restartEM("3");
  check_state_v3();
  var disabled = [];
  try {
    gPrefs.getCharPref(PREF_EM_DISABLED_ADDONS_LIST).split(",");
  }
  catch (e) {}
  do_check_eq(disabled.length, 2);
  do_check_true(disabled.indexOf("bug542391_2@tests.mozilla.org") >= 0);
  do_check_true(disabled.indexOf("bug542391_3@tests.mozilla.org") >= 0);
  gPrefs.clearUserPref(PREF_EM_DISABLED_ADDONS_LIST);

  run_test_2();
}


function run_test_4() {
  restartEM("2");
  check_state_v2();
  var disabled = [];
  try {
    gPrefs.getCharPref(PREF_EM_DISABLED_ADDONS_LIST).split(",");
  }
  catch (e) {}
  do_check_eq(disabled.length, 0);

  finish_test();
}

function finish_test() {
  gEM.uninstallItem("bug542391_1@tests.mozilla.org");
  gEM.uninstallItem("bug542391_2@tests.mozilla.org");
  gEM.uninstallItem("bug542391_3@tests.mozilla.org");
  gEM.uninstallItem("bug542391_4@tests.mozilla.org");
  gEM.uninstallItem("bug542391_5@tests.mozilla.org");

  restartEM();
  do_check_eq(gEM.getItemForID("bug542391_1@tests.mozilla.org"), null);
  do_check_eq(gEM.getItemForID("bug542391_2@tests.mozilla.org"), null);
  do_check_eq(gEM.getItemForID("bug542391_3@tests.mozilla.org"), null);
  do_check_eq(gEM.getItemForID("bug542391_4@tests.mozilla.org"), null);
  do_check_eq(gEM.getItemForID("bug542391_5@tests.mozilla.org"), null);
}
