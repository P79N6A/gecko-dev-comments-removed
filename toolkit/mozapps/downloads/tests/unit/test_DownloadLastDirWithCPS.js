



































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/DownloadLastDir.jsm");
Cu.import("resource://gre/modules/Services.jsm");

do_get_profile();

function run_test() {
  function clearHistory() {
    
    Services.obs.notifyObservers(null, "browser:purge-session-history", "");
  }

  do_check_eq(typeof gDownloadLastDir, "object");
  do_check_eq(gDownloadLastDir.file, null);
  
  let tmpDir = Services.dirsvc.get("TmpD", Ci.nsILocalFile);
  
  let uri1 = Services.io.newURI("http://test1.com/", null, null);
  let uri2 = Services.io.newURI("http://test2.com/", null, null);
  let uri3 = Services.io.newURI("http://test3.com/", null, null);
  let uri4 = Services.io.newURI("http://test4.com/", null, null);

  function newDir() {
    let dir = tmpDir.clone();
    dir.append("testdir" + Math.floor(Math.random() * 10000));
    dir.QueryInterface(Ci.nsILocalFile);
    dir.createUnique(Ci.nsIFile.DIRECTORY_TYPE, 0700);
    return dir;
  }
  
  let dir1 = newDir();
  let dir2 = newDir();
  let dir3 = newDir();

  try {
    { 
      gDownloadLastDir.setFile(null, tmpDir);
      do_check_eq(gDownloadLastDir.file.path, tmpDir.path);
      do_check_neq(gDownloadLastDir.file, tmpDir);
    }

    { 
      
      gDownloadLastDir.setFile(uri1, dir1);
      do_check_eq(gDownloadLastDir.file.path, dir1.path);
      do_check_neq(gDownloadLastDir.file, dir1);
      do_check_eq(gDownloadLastDir.getFile(uri1).path, dir1.path); 
      do_check_neq(gDownloadLastDir.getFile(uri1), dir1);
      do_check_eq(gDownloadLastDir.getFile(uri2).path, dir1.path); 
      do_check_neq(gDownloadLastDir.getFile(uri2), dir1);
      do_check_eq(gDownloadLastDir.getFile(uri3).path, dir1.path); 
      do_check_neq(gDownloadLastDir.getFile(uri3), dir1);
      do_check_eq(gDownloadLastDir.getFile(uri4).path, dir1.path); 
      do_check_neq(gDownloadLastDir.getFile(uri4), dir1);
    }

    { 
      gDownloadLastDir.setFile(uri2, dir2);
      do_check_eq(gDownloadLastDir.file.path, dir2.path);
      do_check_eq(gDownloadLastDir.getFile(uri1).path, dir1.path); 
      do_check_eq(gDownloadLastDir.getFile(uri2).path, dir2.path); 
      do_check_eq(gDownloadLastDir.getFile(uri3).path, dir2.path); 
      do_check_eq(gDownloadLastDir.getFile(uri4).path, dir2.path); 
    }

    { 
      gDownloadLastDir.setFile(uri3, dir3);
      do_check_eq(gDownloadLastDir.file.path, dir3.path);
      do_check_eq(gDownloadLastDir.getFile(uri1).path, dir1.path); 
      do_check_eq(gDownloadLastDir.getFile(uri2).path, dir2.path); 
      do_check_eq(gDownloadLastDir.getFile(uri3).path, dir3.path); 
      do_check_eq(gDownloadLastDir.getFile(uri4).path, dir3.path); 
    }

    { 
      gDownloadLastDir.setFile(uri1, dir2);
      do_check_eq(gDownloadLastDir.file.path, dir2.path);
      do_check_eq(gDownloadLastDir.getFile(uri1).path, dir2.path); 
      do_check_eq(gDownloadLastDir.getFile(uri2).path, dir2.path); 
      do_check_eq(gDownloadLastDir.getFile(uri3).path, dir3.path); 
      do_check_eq(gDownloadLastDir.getFile(uri4).path, dir2.path); 
    }

    { 
      clearHistory();
      do_check_eq(gDownloadLastDir.file, null);
      do_check_eq(Services.contentPrefs.hasPref(uri1, "browser.download.lastDir"), false);
      do_check_eq(gDownloadLastDir.getFile(uri1), null);
      do_check_eq(gDownloadLastDir.getFile(uri2), null);
      do_check_eq(gDownloadLastDir.getFile(uri3), null);
      do_check_eq(gDownloadLastDir.getFile(uri4), null);
    }

    let pb;
    try {
      pb = Cc["@mozilla.org/privatebrowsing;1"].getService(Ci.nsIPrivateBrowsingService);
    } catch (e) {
      print("PB service is not available, bail out");
      return;
    }

    Services.prefs.setBoolPref("browser.privatebrowsing.keep_current_session", true);
    
    { 
      gDownloadLastDir.setFile(null, tmpDir);
      pb.privateBrowsingEnabled = true;
      do_check_eq(gDownloadLastDir.file.path, tmpDir.path);
      do_check_eq(gDownloadLastDir.getFile(uri1).path, tmpDir.path);

      pb.privateBrowsingEnabled = false;
      do_check_eq(gDownloadLastDir.file.path, tmpDir.path);
      do_check_eq(gDownloadLastDir.getFile(uri1).path, tmpDir.path);
      
      clearHistory();
    }

    { 
      gDownloadLastDir.setFile(uri1, dir1);
      pb.privateBrowsingEnabled = true;
      do_check_eq(gDownloadLastDir.file.path, dir1.path);
      do_check_eq(gDownloadLastDir.getFile(uri1).path, dir1.path);

      pb.privateBrowsingEnabled = false;
      do_check_eq(gDownloadLastDir.file.path, dir1.path);
      do_check_eq(gDownloadLastDir.getFile(uri1).path, dir1.path);

      clearHistory();
    }
    
    { 
      pb.privateBrowsingEnabled = true;
      gDownloadLastDir.setFile(null, tmpDir);
      do_check_eq(gDownloadLastDir.file.path, tmpDir.path);
      do_check_eq(gDownloadLastDir.getFile(uri1).path, tmpDir.path);

      pb.privateBrowsingEnabled = false;
      do_check_eq(gDownloadLastDir.file, null);
      do_check_eq(gDownloadLastDir.getFile(uri1), null);
      
      clearHistory();
    }
    
    { 
      pb.privateBrowsingEnabled = true;
      gDownloadLastDir.setFile(uri1, dir1);
      do_check_eq(gDownloadLastDir.file.path, dir1.path);
      do_check_eq(gDownloadLastDir.getFile(uri1).path, dir1.path);

      pb.privateBrowsingEnabled = false;
      do_check_eq(gDownloadLastDir.file, null);
      do_check_eq(gDownloadLastDir.getFile(uri1), null);

      clearHistory();
    }

    { 
      gDownloadLastDir.setFile(uri1, dir1);
      pb.privateBrowsingEnabled = true;
      gDownloadLastDir.setFile(uri1, dir2);
      do_check_eq(gDownloadLastDir.file.path, dir2.path);
      do_check_eq(gDownloadLastDir.getFile(uri1).path, dir2.path);

      pb.privateBrowsingEnabled = false;
      do_check_eq(gDownloadLastDir.file.path, dir1.path);
      do_check_eq(gDownloadLastDir.getFile(uri1).path, dir1.path);

      
      pb.privateBrowsingEnabled = true;
      do_check_eq(gDownloadLastDir.file.path, dir1.path);
      do_check_eq(gDownloadLastDir.getFile(uri1).path, dir1.path);
      
      pb.privateBrowsingEnabled = false;
      clearHistory();
    }
    
    { 
      pb.privateBrowsingEnabled = true;
      gDownloadLastDir.setFile(uri1, dir2);

      clearHistory();
      do_check_eq(gDownloadLastDir.file, null);
      do_check_eq(gDownloadLastDir.getFile(uri1), null);

      pb.privateBrowsingEnabled = false;
      do_check_eq(gDownloadLastDir.file, null);
      do_check_eq(gDownloadLastDir.getFile(uri1), null);
    }
  } finally {
    dir1.remove(true);
    dir2.remove(true);
    dir3.remove(true);
  }
}
