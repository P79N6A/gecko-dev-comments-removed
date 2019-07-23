








































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
    for each (let site in ["delete.me", "i.live"]) {
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

  
  let ww = Cc["@mozilla.org/embedcomp/window-watcher;1"].
           getService(Ci.nsIWindowWatcher);
  ww.registerNotification({
    observe: function(aSubject, aTopic, aData) {
      ww.unregisterNotification(this);
      aSubject.QueryInterface(Ci.nsIDOMEventTarget).
      addEventListener("DOMContentLoaded", doTest, false);
    }
  });

  let testPhase = 0;

  
  let doTest = function() setTimeout(function() {
    win = wm.getMostRecentWindow("Download:Manager");
    let $ = function(id) win.document.getElementById(id);

    let downloadView = $("downloadView");
    let searchbox = $("searchbox");

    
    if (downloadView.selectedIndex)
      return doTest();

    
    switch (testPhase) {
      case 0:
        
        searchbox.value = "delete me";
        searchbox.doCommand();

        
        testPhase++;
        return doTest();
      case 1:
        
        ok(downloadView.itemCount == 1, "Search found the item to delete");

        
        $("cmd_clearList").doCommand();

        
        testPhase++;
        return doTest();
      case 2:
        
        ok(downloadView.itemCount == 1, "Clear list rebuilt the list with one");

        
        return finish();
    }
  }, 0);
 
  
  Cc["@mozilla.org/download-manager-ui;1"].
  getService(Ci.nsIDownloadManagerUI).show();

  waitForExplicitFinish();
}
