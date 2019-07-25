










































const PRIVATEBROWSING_CONTRACT_ID = "@mozilla.org/privatebrowsing;1";

function run_test() {
  
  if (!(PRIVATEBROWSING_CONTRACT_ID in AUS_Cc))
    return;

  do_test_pending();
  dump("Testing: Bug 497578 - begin download of a complete update after a " +
       "failure to apply a partial update with " +
       "browser.privatebrowsing.autostart set to true\n");

  removeUpdateDirsAndFiles();
  setUpdateChannel();

  let registrar = Components.manager.QueryInterface(AUS_Ci.nsIComponentRegistrar);
  registrar.registerFactory(Components.ID("{1dfeb90a-2193-45d5-9cb8-864928b2af55}"),
                            "Fake Window Watcher",
                            "@mozilla.org/embedcomp/window-watcher;1",
                             WindowWatcherFactory);

  
  
  Services.prefs.setBoolPref(PREF_APP_UPDATE_ENABLED, true);
  Services.prefs.setBoolPref("browser.privatebrowsing.autostart", true);

  do_execute_soon(run_test_pt1);
}

function end_test() {
  let registrar = Components.manager.QueryInterface(AUS_Ci.nsIComponentRegistrar);
  registrar.unregisterFactory(Components.ID("{1dfeb90a-2193-45d5-9cb8-864928b2af55}"),
                              WindowWatcherFactory);
  do_test_finished();
  cleanUp();
}

function run_test_pt1() {
  writeUpdatesToXMLFile(getLocalUpdatesXMLString(""), false);
  let url = URL_HOST + URL_PATH + "/partial.mar";
  let patches = getLocalPatchString("partial", url, null, null, null, null,
                                    STATE_FAILED) +
                getLocalPatchString(null, null, null, null, null, null,
                                    STATE_NONE);
  let updates = getLocalUpdateString(patches, null, null, "version 1.0", "1.0",
                                     null, null, null, null, url);
  writeUpdatesToXMLFile(getLocalUpdatesXMLString(updates), true);
  writeStatusFile(STATE_FAILED);

  standardInit();

  dump("Testing: activeUpdate.state should equal STATE_DOWNLOADING prior to " +
       "entering private browsing\n");
  do_check_eq(gUpdateManager.activeUpdate.state, STATE_DOWNLOADING);

  let privBrowsing = AUS_Cc[PRIVATEBROWSING_CONTRACT_ID].
                     getService(AUS_Ci.nsIPrivateBrowsingService).
                     QueryInterface(AUS_Ci.nsIObserver);

  privBrowsing.observe(null, "profile-after-change", "");
  dump("Testing: private mode should be entered automatically\n");
  do_check_true(privBrowsing.privateBrowsingEnabled);

  dump("Testing: private browsing is auto-started\n");
  do_check_true(privBrowsing.autoStarted);

  
  do_execute_soon(run_test_pt2);
}
function run_test_pt2() {
  dump("Testing: update count should equal 1\n");
  do_check_eq(gUpdateManager.updateCount, 1);
  dump("Testing: activeUpdate should not equal null\n");
  do_check_neq(gUpdateManager.activeUpdate, null);
  dump("Testing: activeUpdate.state should not equal null\n");
  do_check_neq(gUpdateManager.activeUpdate.state, null);
  dump("Testing: activeUpdate.state should equal STATE_DOWNLOADING\n");
  do_check_eq(gUpdateManager.activeUpdate.state, STATE_DOWNLOADING);
  end_test();
}


var WindowWatcher = {
  openWindow: function(parent, url, name, features, args) {
  },

  QueryInterface: function(iid) {
    if (iid.equals(AUS_Ci.nsIWindowWatcher) ||
        iid.equals(AUS_Ci.nsISupports))
      return this;

    throw AUS_Cr.NS_ERROR_NO_INTERFACE;
  }
}

var WindowWatcherFactory = {
  createInstance: function createInstance(outer, iid) {
    if (outer != null)
      throw AUS_Cr.NS_ERROR_NO_AGGREGATION;
    return WindowWatcher.QueryInterface(iid);
  }
};
