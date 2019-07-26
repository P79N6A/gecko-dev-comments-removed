














function getExpirablePRTime() {
  let dateObj = new Date();
  
  dateObj.setHours(0);
  dateObj.setMinutes(0);
  dateObj.setSeconds(0);
  dateObj.setMilliseconds(0);
  dateObj = new Date(dateObj.getTime() - 8 * 86400000);
  return dateObj.getTime() * 1000;
}

function run_test()
{
  
  
  if (!("@mozilla.org/browser/nav-history-service;1" in Cc))
    return;

  
  Services.prefs.setBoolPref("places.history.enabled", true);

  let dm = Cc["@mozilla.org/download-manager;1"].
           getService(Ci.nsIDownloadManager);
  let db = dm.DBConnection;

  
  db.executeSimpleSQL("DELETE FROM moz_downloads");

  let stmt = db.createStatement(
    "INSERT INTO moz_downloads (id, source, target, state, guid) " +
    "VALUES (?1, ?2, ?3, ?4, ?5)");

  let iosvc = Cc["@mozilla.org/network/io-service;1"].
              getService(Ci.nsIIOService);
  let theId = 1337;
  let theURI = iosvc.newURI("http://expireme/please", null, null);
  let theGUID = "a1bcD23eF4g5";

  try {
    
    stmt.bindByIndex(0, theId);
    stmt.bindByIndex(1, theURI.spec);

    
    let file = Cc["@mozilla.org/file/directory_service;1"].
               getService(Ci.nsIProperties).get("TmpD", Ci.nsIFile);
    file.append("expireTest");
    stmt.bindByIndex(2, Cc["@mozilla.org/network/io-service;1"].
      getService(Ci.nsIIOService).newFileURI(file).spec);

    
    stmt.bindByIndex(3, dm.DOWNLOAD_FINISHED);
    
    stmt.bindByIndex(4, theGUID);

    
    stmt.execute();
  }
  finally {
    stmt.reset();
    stmt.finalize();
  }

  
  let histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].
                getService(Ci.nsINavHistoryService);
  histsvc.addVisit(theURI, getExpirablePRTime(), null,
                   histsvc.TRANSITION_DOWNLOAD, false, 0);

  
  let histobs = dm.QueryInterface(Ci.nsINavHistoryObserver);
  histobs.onBeginUpdateBatch();

  
  let obs = Cc["@mozilla.org/observer-service;1"].
            getService(Ci.nsIObserverService);
  const kRemoveTopic = "download-manager-remove-download";
  let testObs = {
    observe: function(aSubject, aTopic, aData) {
      if (aTopic != kRemoveTopic)
        return;

      
      let id = aSubject.QueryInterface(Ci.nsISupportsPRUint32);
      do_check_eq(id.data, theId);

      
      histobs.onEndUpdateBatch();
      obs.removeObserver(testObs, kRemoveTopic);
      do_test_finished();
    }
  };
  obs.addObserver(testObs, kRemoveTopic, false);

  
  Services.prefs.setIntPref("places.history.expiration.max_pages", 0);

  
  let expire = Cc["@mozilla.org/places/expiration;1"].getService(Ci.nsIObserver);
  expire.observe(null, "places-debug-start-expiration", -1);

  
  do_test_pending();
}
