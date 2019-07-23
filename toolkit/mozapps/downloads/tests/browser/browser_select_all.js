








































function test()
{
  let dm = Cc["@mozilla.org/download-manager;1"].
           getService(Ci.nsIDownloadManager);
  let db = dm.DBConnection;

  
  db.executeSimpleSQL("DELETE FROM moz_downloads");

  
  let file = Cc["@mozilla.org/file/directory_service;1"].
             getService(Ci.nsIProperties).get("TmpD", Ci.nsIFile);
  file.append("selectAll");
  let filePath = Cc["@mozilla.org/network/io-service;1"].
                 getService(Ci.nsIIOService).newFileURI(file).spec;

  let stmt = db.createStatement(
    "INSERT INTO moz_downloads (target, source, state) " +
    "VALUES (?1, ?2, ?3)");

  let sites = ["mozilla.org", "mozilla.com", "select.all"];
  try {
    for each (let site in sites) {
      stmt.bindStringParameter(0, filePath);
      stmt.bindStringParameter(1, "http://" + site + "/file");
      stmt.bindInt32Parameter(2, dm.DOWNLOAD_FINISHED);

      
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

  let testObs = {
    observe: function(aSubject, aTopic, aData) {
      if (aTopic != DLMGR_UI_DONE)
        return;

      let win = aSubject.QueryInterface(Ci.nsIDOMWindow);
      let $ = function(aId) win.document.getElementById(aId);
      let downloadView = $("downloadView");

      is(downloadView.itemCount, sites.length, "All downloads displayed");

      
      let isMac = navigator.platform.search("Mac") == 0;
      EventUtils.synthesizeKey("a", { metaKey: isMac, ctrlKey: !isMac }, win);
      is(downloadView.selectedCount, sites.length, "All downloads selected");

      
      EventUtils.synthesizeKey("VK_DELETE", {}, win);
      is(downloadView.itemCount, 0, "All downloads removed");

      
      obs.removeObserver(testObs, DLMGR_UI_DONE);
      finish();
    }
  };
  obs.addObserver(testObs, DLMGR_UI_DONE, false);

  
  Cc["@mozilla.org/download-manager-ui;1"].
  getService(Ci.nsIDownloadManagerUI).show();

  waitForExplicitFinish();
}
