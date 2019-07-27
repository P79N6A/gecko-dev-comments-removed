











const URI_EXTENSION_UPDATE_DIALOG     = "chrome://mozapps/content/extensions/update.xul";
const PREF_EM_SHOW_MISMATCH_UI        = "extensions.showMismatchUI";


const PREF_METADATA_LASTUPDATE           = "extensions.getAddons.cache.lastUpdate";
const PREF_METADATA_UPDATETHRESHOLD_SEC  = "extensions.getAddons.cache.updateThreshold";
const DEFAULT_METADATA_UPDATETHRESHOLD_SEC = 172800;  


Services.prefs.setBoolPref("extensions.checkUpdateSecurity", false);

Services.prefs.setBoolPref("extensions.getAddons.cache.enabled", true);

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://testing-common/httpd.js");
Cu.import("resource://testing-common/MockRegistrar.jsm");
var testserver;

const profileDir = gProfD.clone();
profileDir.append("extensions");


var WindowWatcher = {
  expected: false,

  openWindow: function(parent, url, name, features, args) {
    do_check_true(Services.startup.interrupted);
    do_check_eq(url, URI_EXTENSION_UPDATE_DIALOG);
    do_check_true(this.expected);
    this.expected = false;
  },

  QueryInterface: function(iid) {
    if (iid.equals(Ci.nsIWindowWatcher)
     || iid.equals(Ci.nsISupports))
      return this;

    throw Cr.NS_ERROR_NO_INTERFACE;
  }
}

MockRegistrar.register("@mozilla.org/embedcomp/window-watcher;1", WindowWatcher);


function now() {
  return Math.round(Date.now() / 1000);
}


add_task(function* checkFirstMetadata() {
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "1");

  Services.prefs.setBoolPref(PREF_EM_SHOW_MISMATCH_UI, true);

  
  testserver = new HttpServer();
  testserver.registerDirectory("/data/", do_get_file("data"));
  testserver.registerDirectory("/addons/", do_get_file("addons"));
  testserver.start(-1);
  gPort = testserver.identity.primaryPort;
  const BASE_URL  = "http://localhost:" + gPort;
  const GETADDONS_RESULTS = BASE_URL + "/data/test_AddonRepository_cache.xml";
  Services.prefs.setCharPref(PREF_GETADDONS_BYIDS, GETADDONS_RESULTS);

  
  var min1max2 = {
    id: "min1max2@tests.mozilla.org",
    version: "1.0",
    name: "Test addon compatible with v1->v2",
    targetApplications: [{
      id: "xpcshell@tests.mozilla.org",
      minVersion: "1",
      maxVersion: "2"
    }]
  };
  writeInstallRDFForExtension(min1max2, profileDir);

  startupManager();

  
  yield AddonRepository.repopulateCache();
  do_print("Update done, getting last update");
  let lastUpdate = Services.prefs.getIntPref(PREF_METADATA_LASTUPDATE);
  do_check_true(lastUpdate > 0);

  
  let oldUpdate = lastUpdate - 2 * DEFAULT_METADATA_UPDATETHRESHOLD_SEC;
  Services.prefs.setIntPref(PREF_METADATA_LASTUPDATE, oldUpdate);
  yield AddonRepository.repopulateCache();
  do_check_neq(oldUpdate, Services.prefs.getIntPref(PREF_METADATA_LASTUPDATE));
});


add_task(function* upgrade_no_lastupdate() {
  Services.prefs.clearUserPref(PREF_METADATA_LASTUPDATE);

  WindowWatcher.expected = true;
  yield promiseRestartManager("2");
  do_check_false(WindowWatcher.expected);
});


add_task(function* upgrade_old_lastupdate() {
  let oldEnough = now() - DEFAULT_METADATA_UPDATETHRESHOLD_SEC - 1000;
  Services.prefs.setIntPref(PREF_METADATA_LASTUPDATE, oldEnough);

  WindowWatcher.expected = true;
  
  yield promiseRestartManager("1");
  do_check_false(WindowWatcher.expected);
});


add_task(function* upgrade_young_lastupdate() {
  let notOldEnough = now() - DEFAULT_METADATA_UPDATETHRESHOLD_SEC + 1000;
  Services.prefs.setIntPref(PREF_METADATA_LASTUPDATE, notOldEnough);

  WindowWatcher.expected = false;
  yield promiseRestartManager("2");
  do_check_false(WindowWatcher.expected);
});



const TEST_UPDATETHRESHOLD_SEC = 50000;
add_task(function* upgrade_old_pref_lastupdate() {
  Services.prefs.setIntPref(PREF_METADATA_UPDATETHRESHOLD_SEC, TEST_UPDATETHRESHOLD_SEC);

  let oldEnough = now() - TEST_UPDATETHRESHOLD_SEC - 1000;
  Services.prefs.setIntPref(PREF_METADATA_LASTUPDATE, oldEnough);

  WindowWatcher.expected = true;
  yield promiseRestartManager("1");
  do_check_false(WindowWatcher.expected);
});


add_task(function* upgrade_young_pref_lastupdate() {
  let notOldEnough = now() - TEST_UPDATETHRESHOLD_SEC + 1000;
  Services.prefs.setIntPref(PREF_METADATA_LASTUPDATE, notOldEnough);

  WindowWatcher.expected = false;
  yield promiseRestartManager("2");
  do_check_false(WindowWatcher.expected);
});



add_task(function* cleanup() {
  return new Promise((resolve, reject) => {
    testserver.stop(resolve);
  });
});

function run_test() {
  run_next_test();
}
