



































Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

let didFail = false;
var file;

let promptService = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIPromptService]),
  alert: function() {
    ok(didFail, "javascript: uri failed and showed a message");
    file.remove(false);
    finish();
  }
};

Components.manager.QueryInterface(Ci.nsIComponentRegistrar).
registerFactory(Components.ID("{6cc9c9fe-bc0b-432b-a410-253ef8bcc699}"),
  "PromptService", "@mozilla.org/embedcomp/prompt-service;1",
  {
    createInstance: function(aOuter, aIid) {
      if (aOuter != null)
        throw Components.results.NS_ERROR_NO_AGGREGATION;
      return promptService.QueryInterface(aIid);
    }
  });

function test()
{
  let dm = Cc["@mozilla.org/download-manager;1"].
           getService(Ci.nsIDownloadManager);
  let db = dm.DBConnection;

  
  db.executeSimpleSQL("DELETE FROM moz_downloads");

  let stmt = db.createStatement(
    "INSERT INTO moz_downloads (source, target, state) " +
    "VALUES (?1, ?2, ?3)");

  
  stmt.bindStringParameter(0, "javascript:5");

  
  file = Cc["@mozilla.org/file/directory_service;1"].
         getService(Ci.nsIProperties).get("TmpD", Ci.nsIFile);
  file.append("javascriptURI");
  file.createUnique(Ci.nsIFile.NORMAL_FILE_TYPE, 0666);
  stmt.bindStringParameter(1, Cc["@mozilla.org/network/io-service;1"].
    getService(Ci.nsIIOService).newFileURI(file).spec);

  
  stmt.bindInt32Parameter(2, dm.DOWNLOAD_CANCELED);

  
  stmt.execute();
  stmt.finalize();

  let listener = {
    onDownloadStateChange: function(aState, aDownload)
    {
      switch (aDownload.state) {
        case dm.DOWNLOAD_FAILED:
          
          didFail = true;

          ok(true, "javascript: uri failed instead of getting stuck");
          dm.removeListener(listener);
          break;
      }
    }
  };
  dm.addListener(listener);

  
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

  
  let doTest = function() setTimeout(function() {
    win = wm.getMostRecentWindow("Download:Manager");

    
    if (win.document.getElementById("downloadView").selectedIndex)
      return doTest();

    
    EventUtils.synthesizeKey("VK_ENTER", {}, win);
  }, 0);
 
  
  Cc["@mozilla.org/download-manager-ui;1"].
  getService(Ci.nsIDownloadManagerUI).show();

  waitForExplicitFinish();
}
