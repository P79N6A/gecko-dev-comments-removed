





































const checkListener = {
  _onUpdateStartedCalled: false,
  _onUpdateEndedCalled: false,

  
  onUpdateStarted: function onUpdateStarted() {
    this._onUpdateStartedCalled = true;
  },

  
  onUpdateEnded: function onUpdateEnded() {
    this._onUpdateEndedCalled = true;
  },

  
  onAddonUpdateStarted: function onAddonUpdateStarted(aAddon) {
    do_throw("Unexpected call to onAddonUpdateStarted!");
  },

  
  onAddonUpdateEnded: function onAddonUpdateEnded(aAddon, aStatus) {
    do_throw("Unexpected call to onAddonUpdateEnded!");
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
  do_test_finished();
}
