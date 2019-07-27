



Components.utils.import("resource://testing-common/MockRegistrar.jsm");







function run_test() {
  setupTestCommon();

  debugDump("testing nsIUpdatePrompt notifications should not be seen " +
            "when the " + PREF_APP_UPDATE_SILENT + " preference is true");

  Services.prefs.setBoolPref(PREF_APP_UPDATE_SILENT, true);

  let windowWatcherCID =
    MockRegistrar.register("@mozilla.org/embedcomp/window-watcher;1",
                           WindowWatcher);
  do_register_cleanup(() => {
    MockRegistrar.unregister(windowWatcherCID);
  });

  standardInit();

  debugDump("testing showUpdateInstalled should not call openWindow");
  Services.prefs.setBoolPref(PREF_APP_UPDATE_SHOW_INSTALLED_UI, true);

  gCheckFunc = check_showUpdateInstalled;
  gUP.showUpdateInstalled();
  
  
  Assert.ok(true,
            "calling showUpdateInstalled should not attempt to open a window");

  debugDump("testing showUpdateAvailable should not call openWindow");
  writeUpdatesToXMLFile(getLocalUpdatesXMLString(""), false);
  let patches = getLocalPatchString(null, null, null, null, null, null,
                                    STATE_FAILED);
  let updates = getLocalUpdateString(patches);
  writeUpdatesToXMLFile(getLocalUpdatesXMLString(updates), true);
  writeStatusFile(STATE_FAILED);
  reloadUpdateManagerData();

  gCheckFunc = check_showUpdateAvailable;
  let update = gUpdateManager.activeUpdate;
  gUP.showUpdateAvailable(update);
  
  
  Assert.ok(true,
            "calling showUpdateAvailable should not attempt to open a window");

  debugDump("testing showUpdateError should not call getNewPrompter");
  gCheckFunc = check_showUpdateError;
  update.errorCode = WRITE_ERROR;
  gUP.showUpdateError(update);
  
  
  Assert.ok(true,
            "calling showUpdateError should not attempt to open a window");

  doTestFinish();
}

function check_showUpdateInstalled() {
  do_throw("showUpdateInstalled should not have called openWindow!");
}

function check_showUpdateAvailable() {
  do_throw("showUpdateAvailable should not have called openWindow!");
}

function check_showUpdateError() {
  do_throw("showUpdateError should not have seen getNewPrompter!");
}

const WindowWatcher = {
  openWindow: function(aParent, aUrl, aName, aFeatures, aArgs) {
    gCheckFunc();
  },

  getNewPrompter: function(aParent) {
    gCheckFunc();
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIWindowWatcher])
};
