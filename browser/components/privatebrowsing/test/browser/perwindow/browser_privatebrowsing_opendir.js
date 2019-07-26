






var windowsToClose = [];
function testOnWindow(options, callback) {
  var win = OpenBrowserWindow(options);
  win.addEventListener("load", function onLoad() {
    win.removeEventListener("load", onLoad, false);
    windowsToClose.push(win);
    callback(win);
  }, false);
}

registerCleanupFunction(function() {
  windowsToClose.forEach(function(win) {
    win.close();
  });
});

function test() {
  
  waitForExplicitFinish();
  let ds = Cc["@mozilla.org/file/directory_service;1"].
           getService(Ci.nsIProperties);
  let dir1 = ds.get("ProfD", Ci.nsIFile);
  let dir2 = ds.get("TmpD", Ci.nsIFile);
  let file = dir2.clone();
  file.append("pbtest.file");
  file.createUnique(Ci.nsIFile.NORMAL_FILE_TYPE, 0600);

  const kPrefName = "browser.open.lastDir";

  function setupCleanSlate(win) {
    win.gLastOpenDirectory.reset();
    gPrefService.clearUserPref(kPrefName);
  }

  setupCleanSlate(window);

  
  testOnWindow(undefined, function(nonPrivateWindow) {
    setupCleanSlate(nonPrivateWindow);
    testOnWindow({private: true}, function(privateWindow) {
      setupCleanSlate(privateWindow);

      

      
      ok(!nonPrivateWindow.gLastOpenDirectory.path,
         "Last open directory path should be initially empty");
      nonPrivateWindow.gLastOpenDirectory.path = dir2;
      is(nonPrivateWindow.gLastOpenDirectory.path.path, dir2.path,
         "The path should be successfully set");
      nonPrivateWindow.gLastOpenDirectory.path = null;
      is(nonPrivateWindow.gLastOpenDirectory.path.path, dir2.path,
         "The path should be not change when assigning it to null");
      nonPrivateWindow.gLastOpenDirectory.path = dir1;
      is(nonPrivateWindow.gLastOpenDirectory.path.path, dir1.path,
         "The path should be successfully outside of the private browsing mode");

      
      is(privateWindow.gLastOpenDirectory.path.path, dir1.path,
         "The path should not change when entering the private browsing mode");
      privateWindow.gLastOpenDirectory.path = dir2;
      is(privateWindow.gLastOpenDirectory.path.path, dir2.path,
         "The path should successfully change inside the private browsing mode");

      
      is(nonPrivateWindow.gLastOpenDirectory.path.path, dir1.path,
         "The path should be reset to the same path as before entering the private browsing mode");

      setupCleanSlate(nonPrivateWindow);
      setupCleanSlate(privateWindow);

      

      
      ok(!privateWindow.gLastOpenDirectory.path,
         "No original path should exist inside the private browsing mode");
      privateWindow.gLastOpenDirectory.path = dir1;
      is(privateWindow.gLastOpenDirectory.path.path, dir1.path, 
         "The path should be successfully set inside the private browsing mode");
      
      ok(!nonPrivateWindow.gLastOpenDirectory.path,
         "The path set inside the private browsing mode should not leak when leaving that mode");

      setupCleanSlate(nonPrivateWindow);
      setupCleanSlate(privateWindow);

      
      

      gPrefService.setComplexValue(kPrefName, Ci.nsILocalFile, dir1);
      is(nonPrivateWindow.gLastOpenDirectory.path.path, dir1.path,
         "The pref set from last session should take effect outside the private browsing mode");

      setupCleanSlate(nonPrivateWindow);
      setupCleanSlate(privateWindow);

      
      

      gPrefService.setComplexValue(kPrefName, Ci.nsILocalFile, dir1);
      
      is(privateWindow.gLastOpenDirectory.path.path, dir1.path,
         "The pref set from last session should take effect inside the private browsing mode");
      
      is(nonPrivateWindow.gLastOpenDirectory.path.path, dir1.path,
         "The pref set from last session should remain in effect after leaving the private browsing mode");

      setupCleanSlate(nonPrivateWindow);
      setupCleanSlate(privateWindow);

      

      nonPrivateWindow.gLastOpenDirectory.path = file;
      ok(!nonPrivateWindow.gLastOpenDirectory.path,
         "Setting the path to a file shouldn't work when it's originally null");
      nonPrivateWindow.gLastOpenDirectory.path = dir1;
      nonPrivateWindow.gLastOpenDirectory.path = file;
      is(nonPrivateWindow.gLastOpenDirectory.path.path, dir1.path,
         "Setting the path to a file shouldn't work when it's not originally null");

      
      file.remove(false);
      finish();
    });
  });
}
