









































function test()
{
  let dm = Cc["@mozilla.org/download-manager;1"].
           getService(Ci.nsIDownloadManager);

  
  dm.DBConnection.executeSimpleSQL("DELETE FROM moz_downloads");

  let setClose = function(aVal) Cc["@mozilla.org/preferences-service;1"].
    getService(Ci.nsIPrefBranch).
    setBoolPref("browser.download.manager.closeWhenDone", aVal);

  
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
    
    let dmui = Cc["@mozilla.org/download-manager-ui;1"].
               getService(Ci.nsIDownloadManagerUI);
    

    setClose(false);
    finish();
  }, 500);

  
  setClose(true);
  dm.QueryInterface(Ci.nsIObserver).observe(null, "alertclickcallback", null);

  waitForExplicitFinish();
}
