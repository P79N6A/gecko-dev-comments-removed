





































const Ci = Components.interfaces;
const Cc = Components.classes;

function loadUtilsScript() {
  let loader = Cc["@mozilla.org/moz/jssubscript-loader;1"].
               getService(Ci.mozIJSSubScriptLoader);
  loader.loadSubScript("chrome://global/content/contentAreaUtils.js");
}

do_get_profile();

let window = {};
function run_test()
{
  let pb;
  try {
    pb = Cc["@mozilla.org/privatebrowsing;1"].
         getService(Ci.nsIPrivateBrowsingService);
  } catch (e) {
    print("PB service is not available, bail out");
    return;
  }

  loadUtilsScript();

  let prefsService = Cc["@mozilla.org/preferences-service;1"].
                     getService(Ci.nsIPrefService).
                     QueryInterface(Ci.nsIPrefBranch);
  prefsService.setBoolPref("browser.privatebrowsing.keep_current_session", true);
  let prefs = prefsService.getBranch("browser.download.");
  let dirSvc = Cc["@mozilla.org/file/directory_service;1"].
               getService(Ci.nsIProperties);
  let tmpDir = dirSvc.get("TmpD", Ci.nsILocalFile);
  function newDirectory() {
    let dir = tmpDir.clone();
    dir.append("testdir" + Math.floor(Math.random() * 10000));
    dir.createUnique(Ci.nsIFile.DIRECTORY_TYPE, 0700);
    return dir;
  }
  function newFileInDirectory(dir) {
    let file = dir.clone();
    file.append("testfile" + Math.floor(Math.random() * 10000));
    file.createUnique(Ci.nsIFile.DIRECTORY_TYPE, 0600);
    return file;
  }
  let dir1 = newDirectory();
  let dir2 = newDirectory();
  let dir3 = newDirectory();
  let file1 = newFileInDirectory(dir1);
  let file2 = newFileInDirectory(dir2);
  let file3 = newFileInDirectory(dir3);

  
  let fp = {
    appendFilter: function() {},
    appendFilters: function() {},
    init: function() {},
    show: function() Ci.nsIFilePicker.returnOK,
    displayDirectory: null,
    file: file1
  };
  makeFilePicker = function() fp;

  
  delete ContentAreaUtils.stringBundle;
  ContentAreaUtils.stringBundle = {
    GetStringFromName: function() ""
  };

  
  validateFileName = function(foo) foo;

  let params = {
    fpTitleKey: "test",
    fileInfo: new FileInfo("test.txt", "test.txt", "test", "txt", "http://mozilla.org/test.txt"),
    contentType: "text/plain",
    saveMode: SAVEMODE_FILEONLY,
    saveAsType: kSaveAsType_Complete,
    file: null
  };

  prefs.setComplexValue("lastDir", Ci.nsILocalFile, tmpDir);

  do_check_true(getTargetFile(params));
  
  do_check_eq(fp.displayDirectory.path, tmpDir.path);
  
  do_check_eq(prefs.getComplexValue("lastDir", Ci.nsILocalFile).path, dir1.path);
  
  do_check_eq(gDownloadLastDir.file.path, dir1.path);

  pb.privateBrowsingEnabled = true;
  do_check_eq(prefs.getComplexValue("lastDir", Ci.nsILocalFile).path, dir1.path);
  fp.file = file2;
  fp.displayDirectory = null;
  do_check_true(getTargetFile(params));
  
  do_check_eq(fp.displayDirectory.path, dir1.path);
  
  do_check_eq(prefs.getComplexValue("lastDir", Ci.nsILocalFile).path, dir1.path);
  
  do_check_eq(gDownloadLastDir.file.path, dir2.path);

  pb.privateBrowsingEnabled = false;
  
  do_check_eq(gDownloadLastDir.file.path, dir1.path);
  fp.file = file3;
  fp.displayDirectory = null;
  do_check_true(getTargetFile(params));
  
  do_check_eq(fp.displayDirectory.path, dir1.path);
  
  do_check_eq(prefs.getComplexValue("lastDir", Ci.nsILocalFile).path, dir3.path);
  
  do_check_eq(gDownloadLastDir.file.path, dir3.path);

  
  prefsService.clearUserPref("browser.privatebrowsing.keep_current_session");
  [dir1, dir2, dir3].forEach(function(dir) dir.remove(true));
}
