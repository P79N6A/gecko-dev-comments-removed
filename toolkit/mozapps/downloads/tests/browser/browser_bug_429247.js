








































 

function test()
{
  let ios = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);
  let dmFile = Cc["@mozilla.org/file/directory_service;1"].
               getService(Ci.nsIProperties).get("TmpD", Ci.nsIFile);
  dmFile.append("dm-ui-test.file");
  dmFile.createUnique(Ci.nsIFile.NORMAL_FILE_TYPE, 0666);
  let gTestPath = ios.newFileURI(dmFile).spec;
  
  
  const DownloadData = {
    name: "381603.patch",
    source: "https://bugzilla.mozilla.org/attachment.cgi?id=266520",
    target: gTestPath,
    startTime: 1180493839859230,
    endTime: 1180493839859239,
    state: Ci.nsIDownloadManager.DOWNLOAD_FINISHED,
    currBytes: 0, maxBytes: -1, preferredAction: 0, autoResume: 0
  }

  let dm = Cc["@mozilla.org/download-manager;1"].
           getService(Ci.nsIDownloadManager);
  let db = dm.DBConnection;

  
  db.executeSimpleSQL("DELETE FROM moz_downloads");

  let rawStmt = db.createStatement(
    "INSERT INTO moz_downloads (name, source, target, startTime, endTime, " +
      "state, currBytes, maxBytes, preferredAction, autoResume) " +
    "VALUES (:name, :source, :target, :startTime, :endTime, :state, " +
      ":currBytes, :maxBytes, :preferredAction, :autoResume)");
  let stmt = Cc["@mozilla.org/storage/statement-wrapper;1"].
             createInstance(Ci.mozIStorageStatementWrapper)
  stmt.initialize(rawStmt);
  for (let prop in DownloadData)
    stmt.params[prop] = DownloadData[prop];
    
  stmt.execute();
    
  stmt.statement.finalize();

  

  
  let wm = Cc["@mozilla.org/appshell/window-mediator;1"].
           getService(Ci.nsIWindowMediator);
  let win = wm.getMostRecentWindow("Download:Manager");
  if (win) win.close();

  let obs = Cc["@mozilla.org/observer-service;1"].
            getService(Ci.nsIObserverService);
  const DLMGR_UI_DONE = "download-manager-ui-done";

  let testPhase = 0;
  let testObs = {
    observe: function(aSubject, aTopic, aData) {
      if (aTopic != DLMGR_UI_DONE)
        return;

      let win = aSubject.QueryInterface(Ci.nsIDOMWindow);
      let $ = function(aId) win.document.getElementById(aId);
      let downloadView = $("downloadView");
      
      
      
      let invokeCount = 0;
      let counter = function() invokeCount++;

      
      
      
      let defaultForSelectedTest = ["doDefaultForSelected", counter, counter];

      
      let allExpected = [0, "The default action was not performed"];
      
      
      
      downloadView.selectedIndex = 0;
      
      let [func, test, value] = defaultForSelectedTest;
      
      let copy = win[func];
      win[func] = test;
      
      
      let downloadItem = downloadView.getItemAtIndex(0); 
      let boxOffset = downloadItem.boxObject.height + 1;
      
      EventUtils.synthesizeMouse(downloadItem, 0, boxOffset, {clickCount: 2}, win);
      

      
      let [correct, message] = allExpected;
      ok(value() == correct, message);

      
      invokeCount = 0;
      win[func] = copy;

      
      obs.removeObserver(testObs, DLMGR_UI_DONE);
      finish();
    }
  };
  obs.addObserver(testObs, DLMGR_UI_DONE, false);
 
  
  Cc["@mozilla.org/download-manager-ui;1"].
  getService(Ci.nsIDownloadManagerUI).show();

  waitForExplicitFinish();
}
