




function test() {
  let tmpScope = {};
  let downloadModule = {};
  Cu.import("resource://gre/modules/DownloadLastDir.jsm", downloadModule);
  Cu.import("resource://gre/modules/FileUtils.jsm", tmpScope);
  let FileUtils = tmpScope.FileUtils;
  Cu.import("resource://gre/modules/Services.jsm");
  let MockFilePicker = SpecialPowers.MockFilePicker;
  let gDownloadLastDir = new downloadModule.DownloadLastDir(window);

  let pb = Cc["@mozilla.org/privatebrowsing;1"].
           getService(Ci.nsIPrivateBrowsingService);

  MockFilePicker.init(window);
  MockFilePicker.returnValue = Ci.nsIFilePicker.returnOK;

  
  let validateFileNameToRestore = validateFileName;

  Services.prefs.setBoolPref("browser.privatebrowsing.keep_current_session", true);
  let prefs = Services.prefs.getBranch("browser.download.");
  let tmpDir = FileUtils.getDir("TmpD", [], true);
  function newDirectory() {
    let dir = tmpDir.clone();
    dir.append("testdir");
    dir.createUnique(Ci.nsIFile.DIRECTORY_TYPE, 0700);
    return dir;
  }

  function newFileInDirectory(dir) {
    let file = dir.clone();
    file.append("testfile");
    file.createUnique(Ci.nsIFile.DIRECTORY_TYPE, 0600);
    return file;
  }

  let dir1 = newDirectory();
  let dir2 = newDirectory();
  let dir3 = newDirectory();
  let file1 = newFileInDirectory(dir1);
  let file2 = newFileInDirectory(dir2);
  let file3 = newFileInDirectory(dir3);

  
  registerCleanupFunction(function () {
    Services.prefs.clearUserPref("browser.privatebrowsing.keep_current_session");
    Services.prefs.clearUserPref("browser.download.lastDir");
    [dir1, dir2, dir3].forEach(function(dir) dir.remove(true));
    MockFilePicker.cleanup();
    
    validateFileName = validateFileNameToRestore;
    gDownloadLastDir.cleanupPrivateFile();
    delete FileUtils;
  });

  
  




  
  validateFileName = function(foo) foo;

  let params = {
    
    fileInfo: new FileInfo("test.txt", "test.txt", "test", "txt", "http://mozilla.org/test.txt"),
    contentType: "text/plain",
    saveMode: SAVEMODE_FILEONLY,
    saveAsType: kSaveAsType_Complete,
    file: null
  };

  prefs.setComplexValue("lastDir", Ci.nsIFile, tmpDir);
  MockFilePicker.returnFiles = [file1];
  MockFilePicker.displayDirectory = null;
  ok(getTargetFile(params), "Show the file picker dialog with given params");
  
  is(MockFilePicker.displayDirectory.path, tmpDir.path, "file picker should start with browser.download.lastDir");
  
  is(prefs.getComplexValue("lastDir", Ci.nsIFile).path, dir1.path, "LastDir should be modified before entering PB mode");
  
  is(gDownloadLastDir.file.path, dir1.path, "gDownloadLastDir should be usable outside of the PB mode");

  pb.privateBrowsingEnabled = true;
  is(prefs.getComplexValue("lastDir", Ci.nsIFile).path, dir1.path, "LastDir should be that set before PB mode");
  MockFilePicker.returnFiles = [file2];
  MockFilePicker.displayDirectory = null;
  ok(getTargetFile(params), "Show the file picker dialog with the given params");
  
  is(MockFilePicker.displayDirectory.path, dir1.path, "File picker should start with LastDir set before entering PB mode");
  
  is(prefs.getComplexValue("lastDir", Ci.nsIFile).path, dir1.path, "LastDir should not be modified inside PB mode");
  
  is(gDownloadLastDir.file.path, dir2.path, "gDownloadLastDir should be modified");

  pb.privateBrowsingEnabled = false;
  
  is(gDownloadLastDir.file.path, dir1.path, "gDownloadLastDir should be cleared after leaving PB mode");
  MockFilePicker.returnFiles = [file3];
  MockFilePicker.displayDirectory = null;
  ok(getTargetFile(params), "Show the file picker dialog with the given params");
  
  is(MockFilePicker.displayDirectory.path, dir1.path, "File picker should start with LastDir set before PB mode");
  
  is(prefs.getComplexValue("lastDir", Ci.nsIFile).path, dir3.path, "LastDir should be modified after leaving PB mode");
  
  is(gDownloadLastDir.file.path, dir3.path, "gDownloadLastDir should be usable after leaving PB mode");
}
