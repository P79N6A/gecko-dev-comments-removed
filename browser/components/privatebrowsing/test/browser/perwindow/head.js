function whenNewWindowLoaded(aOptions, aCallback) {
  let win = OpenBrowserWindow(aOptions);
  win.addEventListener("load", function onLoad() {
    win.removeEventListener("load", onLoad, false);
    aCallback(win);
  }, false);
}

function newDirectory() {
  let FileUtils =
    Cu.import("resource://gre/modules/FileUtils.jsm", {}).FileUtils;
  let tmpDir = FileUtils.getDir("TmpD", [], true);
  let dir = tmpDir.clone();
  dir.append("testdir");
  dir.createUnique(Ci.nsIFile.DIRECTORY_TYPE, FileUtils.PERMS_DIRECTORY);
  return dir;
}

function newFileInDirectory(aDir) {
  let FileUtils =
    Cu.import("resource://gre/modules/FileUtils.jsm", {}).FileUtils;
  let file = aDir.clone();
  file.append("testfile");
  file.createUnique(Ci.nsIFile.DIRECTORY_TYPE, FileUtils.PERMS_FILE);
  return file;
}

function clearHistory() {
  
  Services.obs.notifyObservers(null, "browser:purge-session-history", "");
}

function _initTest() {
  
  Services.prefs.setIntPref("browser.startup.page", 0);
  registerCleanupFunction(function() Services.prefs.clearUserPref("browser.startup.page"));
}

_initTest();

