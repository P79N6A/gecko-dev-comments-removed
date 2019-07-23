









































function test()
{
  let dm = Cc["@mozilla.org/download-manager;1"].
           getService(Ci.nsIDownloadManager);
  let db = dm.DBConnection;

  
  db.executeSimpleSQL("DELETE FROM moz_downloads");

  let stmt = db.createStatement(
    "INSERT INTO moz_downloads (name, target, source, state, endTime, maxBytes) " +
    "VALUES (?1, ?2, ?3, ?4, ?5, ?6)");

  try {
    for each (let site in ["ed.agadak.net", "mozilla.org"]) {
      stmt.bindStringParameter(0, "Super Pimped Download");
      stmt.bindStringParameter(1, "file://dummy/file");
      stmt.bindStringParameter(2, "http://" + site + "/file");
      stmt.bindInt32Parameter(3, dm.DOWNLOAD_FINISHED);
      stmt.bindInt64Parameter(4, new Date(1985, 7, 2) * 1000);
      stmt.bindInt64Parameter(5, 111222333444);

      
      stmt.execute();
    }
  } finally {
    stmt.reset();
    stmt.finalize();
  }

  
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
      let searchbox = $("searchbox");

      let search = function(aTerms) {
        searchbox.value = aTerms;
        searchbox.doCommand();
      };

      
      switch (testPhase++) {
        case 0:
          
          search("download super pimped 104 GB august 2");

          break;
        case 1:
          
          ok(downloadView.itemCount == 2, "Search matched both downloads");

          
          search("agadak aug 2 downl 104");

          break;
        case 2:
          
          ok(downloadView.itemCount == 1, "Found the single download");

          
          obs.removeObserver(testObs, DLMGR_UI_DONE);
          finish();

          break;
      }
    }
  };
  obs.addObserver(testObs, DLMGR_UI_DONE, false);
 
  
  Cc["@mozilla.org/download-manager-ui;1"].
  getService(Ci.nsIDownloadManagerUI).show();

  waitForExplicitFinish();
}
