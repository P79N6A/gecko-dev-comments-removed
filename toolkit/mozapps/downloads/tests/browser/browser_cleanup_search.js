











































function test()
{
  let dm = Cc["@mozilla.org/download-manager;1"].
           getService(Ci.nsIDownloadManager);
  let db = dm.DBConnection;

  
  db.executeSimpleSQL("DELETE FROM moz_downloads");

  
  let file = Cc["@mozilla.org/file/directory_service;1"].
             getService(Ci.nsIProperties).get("TmpD", Ci.nsIFile);
  file.append("cleanUp");
  let filePath = Cc["@mozilla.org/network/io-service;1"].
                 getService(Ci.nsIIOService).newFileURI(file).spec;

  let stmt = db.createStatement(
    "INSERT INTO moz_downloads (name, target, source, state) " +
    "VALUES (?1, ?2, ?3, ?4)");

  try {
    for each (let site in ["delete.me", "i.live"]) {
      stmt.bindStringParameter(0, "Super Pimped Download");
      stmt.bindStringParameter(1, filePath);
      stmt.bindStringParameter(2, "http://" + site + "/file");
      stmt.bindInt32Parameter(3, dm.DOWNLOAD_FINISHED);

      
      stmt.execute();
    }
  }
  finally {
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
      let clearList = $("clearListButton");

      
      switch (testPhase++) {
        case 0:
          
          is(clearList.disabled, false, "Clear list is enabled for default 2 item view");

          
          searchbox.value = "delete me";
          searchbox.doCommand();

          break;
        case 1:
          
          is(downloadView.itemCount, 1, "Search found the item to delete");
          is(clearList.disabled, false, "Clear list is enabled for search matching 1 item");

          
          clearList.doCommand();

          break;
        case 2:
          
          is(downloadView.itemCount, 1, "Clear list rebuilt the list with one");
          is(clearList.disabled, false, "Clear list still enabled for 1 item in default view");

          
          clearList.doCommand();

          break;
        case 3:
          
          is(downloadView.itemCount, 0, "Clear list killed everything");
          is(clearList.disabled, true, "Clear list is disabled for no items");

          
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
