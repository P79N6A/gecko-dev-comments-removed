








































function test()
{
  let dm = Cc["@mozilla.org/download-manager;1"].
           getService(Ci.nsIDownloadManager);
  let db = dm.DBConnection;

  
  db.executeSimpleSQL("DELETE FROM moz_downloads");

  let stmt = db.createStatement(
    "INSERT INTO moz_downloads (source, target, state, referrer) " +
    "VALUES (?1, ?2, ?3, ?4)");

  
  stmt.bindStringParameter(0, "http://example.com/httpd.js");

  
  let file = Cc["@mozilla.org/file/directory_service;1"].
             getService(Ci.nsIProperties).get("TmpD", Ci.nsIFile);
  file.append("retry");
  file.createUnique(Ci.nsIFile.NORMAL_FILE_TYPE, 0666);
  stmt.bindStringParameter(1, Cc["@mozilla.org/network/io-service;1"].
    getService(Ci.nsIIOService).newFileURI(file).spec);

  
  stmt.bindInt32Parameter(2, dm.DOWNLOAD_CANCELED);

  
  let referrer = "http://referrer.goes/here";
  stmt.bindStringParameter(3, referrer);

  
  stmt.execute();
  stmt.finalize();

  let listener = {
    onDownloadStateChange: function(aState, aDownload)
    {
      switch (aDownload.state) {
        case dm.DOWNLOAD_DOWNLOADING:
          ok(aDownload.referrer.spec == referrer, "Got referrer on download");
          break;
        case dm.DOWNLOAD_FINISHED:
          ok(aDownload.referrer.spec == referrer, "Got referrer on finish");

          dm.removeListener(listener);
          obs.removeObserver(testObs, DLMGR_UI_DONE);
          try { file.remove(false); } catch(e) {  }
          finish();
          break;
      }
    }
  };
  dm.addListener(listener);

  
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
      EventUtils.synthesizeKey("VK_ENTER", {}, win);
    }
  };
  obs.addObserver(testObs, DLMGR_UI_DONE, false);

  
  Cc["@mozilla.org/download-manager-ui;1"].
  getService(Ci.nsIDownloadManagerUI).show();

  waitForExplicitFinish();
}
