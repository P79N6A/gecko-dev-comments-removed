






































gPrefs.setCharPref("extensions.update.url", "http://localhost:4444/");

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
    run_test_pt2();
  },

  
  onAddonUpdateStarted: function onAddonUpdateStarted(aAddon) {
    this._onAddonUpdateStartedCount++;
  },

  
  onAddonUpdateEnded: function onAddonUpdateEnded(aAddon, aStatus) {
    this._onAddonUpdateEndedCount++;
  }
}


do_load_httpd_js();
var testserver;




function run_test() {
  
  testserver = new nsHttpServer();
  testserver.start(4444);

  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "5", "1.9");
  startupEM();
  const Ci = Components.interfaces;
  gEM.update([], 0, Ci.nsIExtensionManager.UPDATE_SYNC_COMPATIBILITY, checkListener);
  do_test_pending();
}

function run_test_pt2() {
  dump("Checking onUpdateStarted\n");
  do_check_true(checkListener._onUpdateStartedCalled);
  dump("Checking onUpdateEnded\n");
  do_check_true(checkListener._onUpdateEndedCalled);
  do_check_eq(checkListener._onAddonUpdateStartedCount, checkListener._onAddonUpdateEndedCount);
  testserver.stop(do_test_finished);
}
