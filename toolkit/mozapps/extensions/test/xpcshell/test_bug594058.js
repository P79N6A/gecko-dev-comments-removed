







Services.prefs.setBoolPref("extensions.checkUpdateSecurity", false);

Services.prefs.setBoolPref("extensions.showMismatchUI", true);

Components.utils.import("resource://testing-common/MockRegistrar.jsm");

const Ci = Components.interfaces;
const extDir = gProfD.clone();
extDir.append("extensions");

var gCachePurged = false;


var WindowWatcher = {
  openWindow: function(parent, url, name, features, args) {
    do_check_false(gCachePurged);
  },

  QueryInterface: function(iid) {
    if (iid.equals(Ci.nsIWindowWatcher)
     || iid.equals(Ci.nsISupports))
      return this;

    throw Components.results.NS_ERROR_NO_INTERFACE;
  }
}

MockRegistrar.register("@mozilla.org/embedcomp/window-watcher;1", WindowWatcher);




function run_test() {
  do_test_pending();
  gCachePurged = false;

  let obs = AM_Cc["@mozilla.org/observer-service;1"].
    getService(AM_Ci.nsIObserverService);
  obs.addObserver({
    observe: function(aSubject, aTopic, aData) {
      gCachePurged = true;
    }
  }, "startupcache-invalidate", false);
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "1");

  startupManager();
  
  do_check_false(gCachePurged);

  installAllFiles([do_get_addon("test_bug594058")], function() {
    restartManager();
    do_check_true(gCachePurged);
    gCachePurged = false;

    
    
    let extFile = extDir.clone();
    let pastTime = extFile.lastModifiedTime - 5000;
    extFile.append("bug594058@tests.mozilla.org");
    setExtensionModifiedTime(extFile, pastTime);
    let otherFile = extFile.clone();
    otherFile.append("directory");
    otherFile.lastModifiedTime = pastTime;
    otherFile.append("file1");
    otherFile.lastModifiedTime = pastTime;

    restartManager();
    gCachePurged = false;

    otherFile.lastModifiedTime = pastTime + 5000;
    restartManager();
    do_check_true(gCachePurged);
    gCachePurged = false;

    restartManager();
    do_check_false(gCachePurged);

    do_test_finished();
  });  
}
