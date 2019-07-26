




let gTests;
function test() {
  waitForExplicitFinish();
  gTests = runTest();
  moveAlong();
}

function moveAlong() {
  try {
    gTests.next();
  } catch (x if x instanceof StopIteration) {
    finish();
  }
}

function waitForPB() {
  function observer(aSubject, aTopic, aData) {
    Services.obs.removeObserver(observer, "last-pb-context-exited", false);
    executeSoon(moveAlong);
  }
  Services.obs.addObserver(observer, "last-pb-context-exited", false);
}

function runTest() {
  let tmpScope = {};
  Cu.import("resource://gre/modules/DownloadLastDir.jsm", tmpScope);
  let gDownloadLastDir = tmpScope.gDownloadLastDir;
  Cu.import("resource://gre/modules/Services.jsm");
  Cu.import("resource://gre/modules/FileUtils.jsm", tmpScope);
  let FileUtils = tmpScope.FileUtils;

  let pb = Cc["@mozilla.org/privatebrowsing;1"].
           getService(Ci.nsIPrivateBrowsingService);

  function clearHistory() {
    
    Services.obs.notifyObservers(null, "browser:purge-session-history", "");
  }

  is(typeof gDownloadLastDir, "object", "gDownloadLastDir should be a valid object");
  is(gDownloadLastDir.file, null, "LastDir pref should be null to start with");

  let tmpDir = FileUtils.getDir("TmpD", [], true);

  let uri1 = Services.io.newURI("http://test1.com/", null, null);
  let uri2 = Services.io.newURI("http://test2.com/", null, null);
  let uri3 = Services.io.newURI("http://test3.com/", null, null);
  let uri4 = Services.io.newURI("http://test4.com/", null, null);

  function newDir() {
    let dir = tmpDir.clone();
    dir.append("testdir");
    dir.createUnique(Ci.nsIFile.DIRECTORY_TYPE, 0700);
    return dir;
  }

  let dir1 = newDir();
  let dir2 = newDir();
  let dir3 = newDir();
  try {
    { 
      gDownloadLastDir.setFile(null, tmpDir);
      is(gDownloadLastDir.file.path, tmpDir.path, "LastDir should point to the tmpDir");
      isnot(gDownloadLastDir.file, tmpDir, "gDownloadLastDir.file should not be pointing to tmpDir");
    }

    { 
      
      gDownloadLastDir.setFile(uri1, dir1);
      is(gDownloadLastDir.file.path, dir1.path, "gDownloadLastDir should return dir1");
      isnot(gDownloadLastDir.file, dir1, "gDownloadLastDir.file should not return dir1");
      is(gDownloadLastDir.getFile(uri1).path, dir1.path, "uri1 should return dir1"); 
      isnot(gDownloadLastDir.getFile(uri1), dir1, "getFile on uri1 should not return dir1");
      is(gDownloadLastDir.getFile(uri2).path, dir1.path, "uri2 should return dir1"); 
      isnot(gDownloadLastDir.getFile(uri2), dir1, "getFile on uri2 should not return dir1");
      is(gDownloadLastDir.getFile(uri3).path, dir1.path, "uri3 should return dir1"); 
      isnot(gDownloadLastDir.getFile(uri3), dir1, "getFile on uri3 should not return dir1");
      is(gDownloadLastDir.getFile(uri4).path, dir1.path, "uri4 should return dir1"); 
      isnot(gDownloadLastDir.getFile(uri4), dir1, "getFile on uri4 should not return dir1");
    }

    { 
      gDownloadLastDir.setFile(uri2, dir2);
      is(gDownloadLastDir.file.path, dir2.path, "gDownloadLastDir should point to dir2");
      is(gDownloadLastDir.getFile(uri1).path, dir1.path, "uri1 should return dir1"); 
      is(gDownloadLastDir.getFile(uri2).path, dir2.path, "uri2 should return dir2"); 
      is(gDownloadLastDir.getFile(uri3).path, dir2.path, "uri3 should return dir2"); 
      is(gDownloadLastDir.getFile(uri4).path, dir2.path, "uri4 should return dir2"); 
    }

    { 
      gDownloadLastDir.setFile(uri3, dir3);
      is(gDownloadLastDir.file.path, dir3.path, "gDownloadLastDir should point to dir3");
      is(gDownloadLastDir.getFile(uri1).path, dir1.path, "uri1 should return dir1"); 
      is(gDownloadLastDir.getFile(uri2).path, dir2.path, "uri2 should return dir2"); 
      is(gDownloadLastDir.getFile(uri3).path, dir3.path, "uri3 should return dir3"); 
      is(gDownloadLastDir.getFile(uri4).path, dir3.path, "uri4 should return dir4"); 
    }

    { 
      gDownloadLastDir.setFile(uri1, dir2);
      is(gDownloadLastDir.file.path, dir2.path, "gDownloadLastDir should point to dir2");
      is(gDownloadLastDir.getFile(uri1).path, dir2.path, "uri1 should return dir2"); 
      is(gDownloadLastDir.getFile(uri2).path, dir2.path, "uri2 should return dir2"); 
      is(gDownloadLastDir.getFile(uri3).path, dir3.path, "uri3 should return dir3"); 
      is(gDownloadLastDir.getFile(uri4).path, dir2.path, "uri4 should return dir2"); 
    }

    { 
      clearHistory();
      is(gDownloadLastDir.file, null, "clearHistory removes all data");
      is(Services.contentPrefs.hasPref(uri1, "browser.download.lastDir"), false, "LastDir preference should be absent");
      is(gDownloadLastDir.getFile(uri1), null, "uri1 should point to null");
      is(gDownloadLastDir.getFile(uri2), null, "uri2 should point to null");
      is(gDownloadLastDir.getFile(uri3), null, "uri3 should point to null");
      is(gDownloadLastDir.getFile(uri4), null, "uri4 should point to null");
    }

    Services.prefs.setBoolPref("browser.privatebrowsing.keep_current_session", true);

    { 
      gDownloadLastDir.setFile(null, tmpDir);
      pb.privateBrowsingEnabled = true;
      is(gDownloadLastDir.file.path, tmpDir.path, "LastDir should point to tmpDir inside PB mode");
      is(gDownloadLastDir.getFile(uri1).path, tmpDir.path, "uri1 should return tmpDir inside PB mode");

      waitForPB();
      pb.privateBrowsingEnabled = false;
      yield;
      is(gDownloadLastDir.file.path, tmpDir.path, "LastDir should point to tmpDir outside PB mode");
      is(gDownloadLastDir.getFile(uri1).path, tmpDir.path, "uri1 should return tmpDir outside PB mode");

      clearHistory();
    }

    { 
      gDownloadLastDir.setFile(uri1, dir1);
      pb.privateBrowsingEnabled = true;
      is(gDownloadLastDir.file.path, dir1.path, "LastDir should point to dir1 inside PB mode");
      is(gDownloadLastDir.getFile(uri1).path, dir1.path, "uri1 should return dir1 inside PB mode");

      waitForPB();
      pb.privateBrowsingEnabled = false;
      yield;
      is(gDownloadLastDir.file.path, dir1.path, "LastDir should point to dir1 outside PB mode");
      is(gDownloadLastDir.getFile(uri1).path, dir1.path, "uri1 should return dir1 outside PB mode");

      clearHistory();
    }

    { 
      pb.privateBrowsingEnabled = true;
      gDownloadLastDir.setFile(null, tmpDir);
      is(gDownloadLastDir.file.path, tmpDir.path, "LastDir should return tmpDir inside PB mode");
      is(gDownloadLastDir.getFile(uri1).path, tmpDir.path, "uri1 should return tmpDir inside PB mode");

      waitForPB();
      pb.privateBrowsingEnabled = false;
      yield;
      is(gDownloadLastDir.file, null, "LastDir should be null outside PB mode");
      is(gDownloadLastDir.getFile(uri1), null, "uri1 should return null outside PB mode");

      clearHistory();
    }

    { 
      pb.privateBrowsingEnabled = true;
      gDownloadLastDir.setFile(uri1, dir1);
      is(gDownloadLastDir.file.path, dir1.path, "LastDir should point to dir1 inside PB mode");
      is(gDownloadLastDir.getFile(uri1).path, dir1.path, "uri1 should return dir1 inside PB mode");

      waitForPB();
      pb.privateBrowsingEnabled = false;
      yield;
      is(gDownloadLastDir.file, null, "LastDir should point to null outside PB mode");
      is(gDownloadLastDir.getFile(uri1), null, "uri1 should return null outside PB mode");

      clearHistory();
    }

    { 
      gDownloadLastDir.setFile(uri1, dir1);
      pb.privateBrowsingEnabled = true;
      gDownloadLastDir.setFile(uri1, dir2);
      is(gDownloadLastDir.file.path, dir2.path, "LastDir should point to dir2 inside PB mode");
      is(gDownloadLastDir.getFile(uri1).path, dir2.path, "uri1 should return dir2 inside PB mode");

      waitForPB();
      pb.privateBrowsingEnabled = false;
      yield;
      is(gDownloadLastDir.file.path, dir1.path, "LastDir should point to dir1 outside PB mode");
      is(gDownloadLastDir.getFile(uri1).path, dir1.path, "uri1 should return dir1 outside PB mode");

      
      pb.privateBrowsingEnabled = true;
      is(gDownloadLastDir.file.path, dir1.path, "LastDir should be cleared");
      is(gDownloadLastDir.getFile(uri1).path, dir1.path, "uri1 should return dir1");

      waitForPB();
      pb.privateBrowsingEnabled = false;
      yield;
      clearHistory();
    }

    { 
      pb.privateBrowsingEnabled = true;
      gDownloadLastDir.setFile(uri1, dir2);

      clearHistory();
      is(gDownloadLastDir.file, null, "LastDir should be null afer clearing history");
      is(gDownloadLastDir.getFile(uri1), null, "uri1 should return null");

      waitForPB();
      pb.privateBrowsingEnabled = false;
      yield;
      is(gDownloadLastDir.file, null, "LastDir should be null");
      is(gDownloadLastDir.getFile(uri1), null, "uri1 should return null");
    }

    { 
      Services.prefs.setBoolPref("browser.download.lastDir.savePerSite", false);

      gDownloadLastDir.setFile(uri1, dir1);
      is(gDownloadLastDir.file.path, dir1.path, "LastDir should be set to dir1");
      is(gDownloadLastDir.getFile(uri1).path, dir1.path, "uri1 should return dir1");
      is(gDownloadLastDir.getFile(uri2).path, dir1.path, "uri2 should return dir1");
      is(gDownloadLastDir.getFile(uri3).path, dir1.path, "uri3 should return dir1");
      is(gDownloadLastDir.getFile(uri4).path, dir1.path, "uri4 should return dir1");

      gDownloadLastDir.setFile(uri2, dir2);
      is(gDownloadLastDir.file.path, dir2.path, "LastDir should be set to dir2");
      is(gDownloadLastDir.getFile(uri1).path, dir2.path, "uri1 should return dir2");
      is(gDownloadLastDir.getFile(uri2).path, dir2.path, "uri2 should return dir2");
      is(gDownloadLastDir.getFile(uri3).path, dir2.path, "uri3 should return dir2");
      is(gDownloadLastDir.getFile(uri4).path, dir2.path, "uri4 should return dir2");

      Services.prefs.clearUserPref("browser.download.lastDir.savePerSite");
    }

    { 
      gDownloadLastDir.setFile(uri3, dir3);
      is(gDownloadLastDir.getFile(uri3).path, dir3.path, "LastDir should be set to dir3");
      gDownloadLastDir.setFile(uri3, null);
      is(gDownloadLastDir.getFile(uri3), null, "uri3 should return null");
    }
  } finally {
      dir1.remove(true);
      dir2.remove(true);
      dir3.remove(true);
      Services.prefs.clearUserPref("browser.download.lastDir.savePerSite");
      Services.prefs.clearUserPref("browser.download.lastDir");
      gDownloadLastDir.cleanupPrivateFile();
  }
}
