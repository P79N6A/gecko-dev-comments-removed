




function test() {
  waitForExplicitFinish();

  let FileUtils =
    Cu.import("resource://gre/modules/FileUtils.jsm", {}).FileUtils;
  let DownloadLastDir =
    Cu.import("resource://gre/modules/DownloadLastDir.jsm", {}).DownloadLastDir;
  let MockFilePicker = SpecialPowers.MockFilePicker;

  MockFilePicker.init(window);
  MockFilePicker.returnValue = Ci.nsIFilePicker.returnOK;

  let validateFileNameToRestore = validateFileName;
  let prefs = Services.prefs.getBranch("browser.download.");
  let tmpDir = FileUtils.getDir("TmpD", [], true);
  let dir1 = newDirectory();
  let dir2 = newDirectory();
  let dir3 = newDirectory();
  let file1 = newFileInDirectory(dir1);
  let file2 = newFileInDirectory(dir2);
  let file3 = newFileInDirectory(dir3);

  
  registerCleanupFunction(function () {
    Services.prefs.clearUserPref("browser.download.lastDir");
    [dir1, dir2, dir3].forEach(function(dir) dir.remove(true));
    MockFilePicker.cleanup();
    validateFileName = validateFileNameToRestore;
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

  function testOnWindow(aPrivate, aCallback) {
    whenNewWindowLoaded({private: aPrivate}, function(win) {
      let gDownloadLastDir = new DownloadLastDir(win);
      aCallback(win, gDownloadLastDir);
      gDownloadLastDir.cleanupPrivateFile();
      win.close();
    });
  }

  function testDownloadDir(aWin, gDownloadLastDir, aFile, aDisplayDir, aLastDir,
                           aGlobalLastDir, aCallback) {
    
    is(prefs.getComplexValue("lastDir", Ci.nsIFile).path, aDisplayDir.path,
       "LastDir should be the expected display dir");
    
    is(gDownloadLastDir.file.path, aDisplayDir.path,
       "gDownloadLastDir should be the expected display dir");

    MockFilePicker.returnFiles = [aFile];
    MockFilePicker.displayDirectory = null;
    aWin.getTargetFile(params, function() {
      
      is(MockFilePicker.displayDirectory.path, aDisplayDir.path,
         "File picker should start with browser.download.lastDir");
      
      is(prefs.getComplexValue("lastDir", Ci.nsIFile).path, aLastDir.path,
         "LastDir should be the expected last dir");
      
      is(gDownloadLastDir.file.path, aGlobalLastDir.path,
         "gDownloadLastDir should be the expected global last dir");

      aCallback();
    });
  }

  testOnWindow(false, function(win, downloadDir) {
    testDownloadDir(win, downloadDir, file1, tmpDir, dir1, dir1, function() {
      testOnWindow(true, function(win, downloadDir) {
        testDownloadDir(win, downloadDir, file2, dir1, dir1, dir2, function() {
          testOnWindow(false, function(win, downloadDir) {
            testDownloadDir(win, downloadDir, file3, dir1, dir3, dir3, finish);
          });
        });
      });
    });
  });
}
