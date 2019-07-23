





































function bug413093obs()
{
  this.mDownload = null;
  this.wasPaused = false;
}
bug413093obs.prototype = {
  observe: function(aSubject, aTopic, aData)
  {
    if ("domwindowopened" == aTopic) {
      let ww = Cc["@mozilla.org/embedcomp/window-watcher;1"].
               getService(Ci.nsIWindowWatcher);
      ww.unregisterNotification(this);

      
      
      if (!this.wasPaused) {
        dump("domwindowopened callback - not pausing or resuming\n");
        this.wasPaused = true;
        return;
      }

      dump("domwindowopened callback - resuming download\n");
      
      
      let dm = Cc["@mozilla.org/download-manager;1"].
                getService(Ci.nsIDownloadManager);
      dm.resumeDownload(this.mDownload.id);
    } else if ("timer-callback" == aTopic) {
      this.setPref(false);

      let dmui = Cc["@mozilla.org/download-manager-ui;1"].
                 getService(Ci.nsIDownloadManagerUI);
      ok(!dmui.visible, "Download Manager UI is not showing");

      finish();
    }
  },

  onDownloadStateChange: function(aOldState, aDownload)
  {
    if (aDownload.state == Ci.nsIDownloadManager.DOWNLOAD_DOWNLOADING &&
        !this.wasPaused) {
      dump("onDownloadStateChange - pausing download\n");
      this.wasPaused = true;

      
      let dm = Cc["@mozilla.org/download-manager;1"].
                getService(Ci.nsIDownloadManager);
      dm.pauseDownload(aDownload.id);
    }

    if (aDownload.state == Ci.nsIDownloadManager.DOWNLOAD_FINISHED) {
      aDownload.targetFile.remove(false);

      let dm = Cc["@mozilla.org/download-manager;1"].
                getService(Ci.nsIDownloadManager);
      dm.removeListener(this);

      
      
      let timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
      timer.init(this, 1000, Ci.nsITimer.TYPE_ONE_SHOT);
    }
  },
  onStateChange: function(a, b, c, d, e) { },
  onProgressChange: function(a, b, c, d, e, f, g) { },
  onSecurityChange: function(a, b, c, d) { },

  setPref: function(aDoTest)
  {
    let prefs = Cc["@mozilla.org/preferences-service;1"].
                getService(Ci.nsIPrefBranch);

    
    prefs.setIntPref("browser.download.manager.retention", aDoTest ? 0 : 2);
    prefs.setBoolPref("browser.download.manager.closeWhenDone", aDoTest);
  }
};
function test()
{
  function addDownload() {
    function createURI(aObj) {
      let ios = Cc["@mozilla.org/network/io-service;1"].
                getService(Ci.nsIIOService);
      return (aObj instanceof Ci.nsIFile) ? ios.newFileURI(aObj) :
                                            ios.newURI(aObj, null, null);
    }

    const nsIWBP = Ci.nsIWebBrowserPersist;
    let persist = Cc["@mozilla.org/embedding/browser/nsWebBrowserPersist;1"]
                  .createInstance(Ci.nsIWebBrowserPersist);
    persist.persistFlags = nsIWBP.PERSIST_FLAGS_REPLACE_EXISTING_FILES |
                           nsIWBP.PERSIST_FLAGS_BYPASS_CACHE |
                           nsIWBP.PERSIST_FLAGS_AUTODETECT_APPLY_CONVERSION;

    let dirSvc = Cc["@mozilla.org/file/directory_service;1"].
                 getService(Ci.nsIProperties);
    let destFile = dirSvc.get("ProfD", Ci.nsIFile);
    destFile.append("download.result");
    if (destFile.exists())
      destFile.remove(false);

    let src = createURI("http://example.com/httpd.js");
    let target = createURI(destFile);
    let tr = Cc["@mozilla.org/transfer;1"].createInstance(Ci.nsITransfer);
    tr.init(src, target, "test download", null, Math.round(Date.now() * 1000),
            null, persist);
    persist.progressListener = tr;
    persist.saveURI(src, null, null, null, null, destFile);
  }

  
  let dm = Cc["@mozilla.org/download-manager;1"].
           getService(Ci.nsIDownloadManager);
  dm.DBConnection.executeSimpleSQL("DELETE FROM moz_downloads");

  
  let wm = Cc["@mozilla.org/appshell/window-mediator;1"].
           getService(Ci.nsIWindowMediator);
  let win = wm.getMostRecentWindow("Download:Manager");
  if (win)
    win.close();

  let obs = new bug413093obs();
  dm.addListener(obs);
  obs.setPref(true);

  
  let ww = Cc["@mozilla.org/embedcomp/window-watcher;1"].
           getService(Ci.nsIWindowWatcher);
  ww.registerNotification(obs);

  addDownload();

  waitForExplicitFinish();
}
