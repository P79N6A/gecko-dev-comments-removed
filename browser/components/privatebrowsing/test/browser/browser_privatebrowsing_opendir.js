







































function test() {
  
  let pb = Cc["@mozilla.org/privatebrowsing;1"].
           getService(Ci.nsIPrivateBrowsingService);
  let ds = Cc["@mozilla.org/file/directory_service;1"].
           getService(Ci.nsIProperties);
  let dir1 = ds.get("ProfD", Ci.nsIFile);
  let dir2 = ds.get("TmpD", Ci.nsIFile);
  let file = dir2.clone();
  file.append("pbtest.file");
  file.createUnique(Ci.nsIFile.NORMAL_FILE_TYPE, 0600);

  const kPrefName = "browser.open.lastDir";

  function setupCleanSlate() {
    gLastOpenDirectory.reset();
    if (gPrefService.prefHasUserValue(kPrefName))
        gPrefService.clearUserPref(kPrefName);
  }

  setupCleanSlate();

  

  
  ok(!gLastOpenDirectory.path,
     "Last open directory path should be initially empty");
  gLastOpenDirectory.path = dir2;
  is(gLastOpenDirectory.path.path, dir2.path,
     "The path should be successfully set");
  gLastOpenDirectory.path = null;
  is(gLastOpenDirectory.path.path, dir2.path,
     "The path should be not change when assigning it to null");
  gLastOpenDirectory.path = dir1;
  is(gLastOpenDirectory.path.path, dir1.path,
     "The path should be successfully outside of the private browsing mode");

  
  pb.privateBrowsingEnabled = true;

  is(gLastOpenDirectory.path.path, dir1.path,
     "The path should not change when entering the private browsing mode");
  gLastOpenDirectory.path = dir2;
  is(gLastOpenDirectory.path.path, dir2.path,
     "The path should successfully change inside the private browsing mode");

  
  pb.privateBrowsingEnabled = false;

  is(gLastOpenDirectory.path.path, dir1.path,
     "The path should be reset to the same path as before entering the private browsing mode");

  setupCleanSlate();

  

  pb.privateBrowsingEnabled = true;
  ok(!gLastOpenDirectory.path,
     "No original path should exist inside the private browsing mode");
  gLastOpenDirectory.path = dir1;
  is(gLastOpenDirectory.path.path, dir1.path, 
     "The path should be successfully set inside the private browsing mode");
  pb.privateBrowsingEnabled = false;
  ok(!gLastOpenDirectory.path,
     "The path set inside the private browsing mode should not leak when leaving that mode");

  setupCleanSlate();

  
  

  gPrefService.setComplexValue(kPrefName, Ci.nsILocalFile, dir1);
  is(gLastOpenDirectory.path.path, dir1.path,
     "The pref set from last session should take effect outside the private browsing mode");

  setupCleanSlate();

  
  

  gPrefService.setComplexValue(kPrefName, Ci.nsILocalFile, dir1);
  pb.privateBrowsingEnabled = true;
  is(gLastOpenDirectory.path.path, dir1.path,
     "The pref set from last session should take effect inside the private browsing mode");
  pb.privateBrowsingEnabled = false;
  is(gLastOpenDirectory.path.path, dir1.path,
     "The pref set from last session should remain in effect after leaving the private browsing mode");

  setupCleanSlate();

  

  gLastOpenDirectory.path = file;
  ok(!gLastOpenDirectory.path,
     "Setting the path to a file shouldn't work when it's originally null");
  gLastOpenDirectory.path = dir1;
  gLastOpenDirectory.path = file;
  is(gLastOpenDirectory.path.path, dir1.path,
     "Setting the path to a file shouldn't work when it's not originally null");

  
  file.remove(false);
}
