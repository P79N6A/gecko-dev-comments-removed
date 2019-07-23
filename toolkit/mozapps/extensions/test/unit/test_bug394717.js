





































const checkListener = {
  _onUpdateStartedCalled: false,
  _onUpdateEndedCalled: false,
  _onAddonUpdateStartedCount: 0,
  _onAddonUpdateEndedCount: 0,

  
  onUpdateStarted: function onUpdateStarted() {
    this._onUpdateStartedCalled = true;
  },

  
  onUpdateEnded: function onUpdateEnded() {
    this._onUpdateEndedCalled = true;
  },

  
  onAddonUpdateStarted: function onAddonUpdateStarted(aAddon) {
    this._onAddonUpdateStartedCount++;
  },

  
  onAddonUpdateEnded: function onAddonUpdateEnded(aAddon, aStatus) {
    this._onAddonUpdateEndedCount++;
  }
}




function run_test() {
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "5", "1.9");
  startupEM();
  const Ci = Components.interfaces;
  gEM.update([], 0, Ci.nsIExtensionManager.UPDATE_SYNC_COMPATIBILITY, checkListener);
  do_test_pending();
  do_timeout(5000, "run_test_pt2()");
}

function run_test_pt2() {
  dump("Checking onUpdateStarted\n");
  do_check_true(checkListener._onUpdateStartedCalled);
  dump("Checking onUpdateEnded\n");
  do_check_true(checkListener._onUpdateEndedCalled);
  do_check_eq(checkListener._onAddonUpdateStartedCount, checkListener._onAddonUpdateEndedCount);
  do_test_finished();
}
