





































const Ci = Components.interfaces;
const Cc = Components.classes;
const Cu = Components.utils;
const Cr = Components.results;

do_get_profile();

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/DownloadLastDir.jsm");

let context = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIInterfaceRequestor]),
  getInterface: XPCOMUtils.generateQI([Ci.nsIDOMWindow])
};

let launcher = {
  source: Services.io.newURI("http://test1.com/file", null, null)
};

Cu.import("resource://test/MockFilePicker.jsm");
MockFilePicker.reset();
MockFilePicker.returnValue = Ci.nsIFilePicker.returnOK;

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

  let prefsService = Cc["@mozilla.org/preferences-service;1"].
                     getService(Ci.nsIPrefService).
                     QueryInterface(Ci.nsIPrefBranch);
  prefsService.setBoolPref("browser.privatebrowsing.keep_current_session", true);
  let prefs = prefsService.getBranch("browser.download.");
  let launcherDialog = Cc["@mozilla.org/helperapplauncherdialog;1"].
                       getService(Ci.nsIHelperAppLauncherDialog);
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

  prefs.setComplexValue("lastDir", Ci.nsILocalFile, tmpDir);

  MockFilePicker.returnFiles = [file1];
  let file = launcherDialog.promptForSaveToFile(launcher, context, null, null, null);
  do_check_true(!!file);
  
  do_check_eq(MockFilePicker.displayDirectory.path, tmpDir.path);
  
  do_check_eq(prefs.getComplexValue("lastDir", Ci.nsILocalFile).path, dir1.path);
  
  do_check_eq(gDownloadLastDir.file.path, dir1.path);

  pb.privateBrowsingEnabled = true;
  do_check_eq(prefs.getComplexValue("lastDir", Ci.nsILocalFile).path, dir1.path);
  MockFilePicker.returnFiles = [file2];
  MockFilePicker.displayDirectory = null;
  file = launcherDialog.promptForSaveToFile(launcher, context, null, null, null);
  do_check_true(!!file);
  
  do_check_eq(MockFilePicker.displayDirectory.path, dir1.path);
  
  do_check_eq(prefs.getComplexValue("lastDir", Ci.nsILocalFile).path, dir1.path);
  
  do_check_eq(gDownloadLastDir.file.path, dir2.path);

  pb.privateBrowsingEnabled = false;
  
  do_check_eq(gDownloadLastDir.file.path, dir1.path);
  MockFilePicker.returnFiles = [file3];
  MockFilePicker.displayDirectory = null;
  file = launcherDialog.promptForSaveToFile(launcher, context, null, null, null);
  do_check_true(!!file);
  
  do_check_eq(MockFilePicker.displayDirectory.path, dir1.path);
  
  do_check_eq(prefs.getComplexValue("lastDir", Ci.nsILocalFile).path, dir3.path);
  
  do_check_eq(gDownloadLastDir.file.path, dir3.path);

  
  prefsService.clearUserPref("browser.privatebrowsing.keep_current_session");
  [dir1, dir2, dir3].forEach(function(dir) dir.remove(true));

  MockFilePicker.reset();
}
