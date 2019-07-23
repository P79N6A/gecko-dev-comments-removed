







































function run_test()
{
  let dm = Cc["@mozilla.org/download-manager;1"].
           getService(Ci.nsIDownloadManager);
  let db = dm.DBConnection;

  
  db.executeSimpleSQL("DELETE FROM moz_downloads");

  let stmt = db.createStatement(
    "INSERT INTO moz_downloads (id, source, target, state) " +
    "VALUES (?1, ?2, ?3, ?4)");

  let iosvc = Cc["@mozilla.org/network/io-service;1"].
              getService(Ci.nsIIOService);
  let theId = 1337;
  let theURI = iosvc.newURI("http://expireme/please", null, null);

  try {
    
    stmt.bindInt32Parameter(0, theId);
    stmt.bindStringParameter(1, theURI.spec);

    
    let file = Cc["@mozilla.org/file/directory_service;1"].
               getService(Ci.nsIProperties).get("TmpD", Ci.nsIFile);
    file.append("expireTest");
    stmt.bindStringParameter(2, Cc["@mozilla.org/network/io-service;1"].
      getService(Ci.nsIIOService).newFileURI(file).spec);

    
    stmt.bindInt32Parameter(3, dm.DOWNLOAD_FINISHED);

    
    stmt.execute();
  }
  finally {
    stmt.reset();
    stmt.finalize();
  }

  let histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].
                getService(Ci.nsINavHistoryService);
  
  histsvc.addVisit(theURI, Date.now() * 1000, null,
                   histsvc.TRANSITION_DOWNLOAD, false, 0);

  
  let obs = Cc["@mozilla.org/observer-service;1"].
            getService(Ci.nsIObserverService);
  const kRemoveTopic = "download-manager-remove-download";
  let testObs = {
    observe: function(aSubject, aTopic, aData) {
      if (aTopic != kRemoveTopic)
        return;

      
      let id = aSubject.QueryInterface(Ci.nsISupportsPRUint32);
      do_check_eq(id.data, theId);

      
      obs.removeObserver(testObs, kRemoveTopic);
      do_test_finished();
    }
  };
  obs.addObserver(testObs, kRemoveTopic, false);

  
  let prefs = Cc["@mozilla.org/preferences-service;1"].
              getService(Ci.nsIPrefBranch);
  prefs.setIntPref("browser.history_expire_sites", 0);
  prefs.setIntPref("browser.history_expire_days_min", 0);
  prefs.setIntPref("browser.history_expire_days", 0);

  
  do_test_pending();
}
