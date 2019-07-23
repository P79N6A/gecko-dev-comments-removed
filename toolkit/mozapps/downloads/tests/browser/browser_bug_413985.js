




































function bug413985obs(aWin)
{
  this.mWin = aWin;
  this.wasPaused = false;
  this.wasResumed = false;
}
bug413985obs.prototype = {
  observe: function(aSubject, aTopic, aData)
  {
    if ("timer-callback" == aTopic) {
      
      EventUtils.synthesizeKey(" ", {}, this.mWin);
    }
  },

  onDownloadStateChange: function(aState, aDownload)
  {
    if (aDownload.state == Ci.nsIDownloadManager.DOWNLOAD_DOWNLOADING &&
        !this.wasPaused) {
      this.wasPaused = true;
      
      EventUtils.synthesizeKey(" ", {}, this.mWin);
    }

    if (aDownload.state == Ci.nsIDownloadManager.DOWNLOAD_PAUSED &&
        !this.wasResumed) {
      this.wasResumed = true;
      
      
      var timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
      timer.init(this, 0, Ci.nsITimer.TYPE_ONE_SHOT);
    }

    if (aDownload.state == Ci.nsIDownloadManager.DOWNLOAD_FINISHED) {
      ok(this.wasPaused && this.wasResumed,
         "The download was paused, and then resumed to completion");
      aDownload.targetFile.remove(false);

      var dm = Cc["@mozilla.org/download-manager;1"].
                getService(Ci.nsIDownloadManager);
      dm.removeListener(this);

      finish();
    }
  },
  onStateChange: function(a, b, c, d, e) { },
  onProgressChange: function(a, b, c, d, e, f, g) { },
  onSecurityChange: function(a, b, c, d) { }
};
function test()
{
  var dm = Cc["@mozilla.org/download-manager;1"].
           getService(Ci.nsIDownloadManager);

  function addDownload() {
    function createURI(aObj) {
      var ios = Cc["@mozilla.org/network/io-service;1"].
                getService(Ci.nsIIOService);
      return (aObj instanceof Ci.nsIFile) ? ios.newFileURI(aObj) :
                                            ios.newURI(aObj, null, null);
    }

    const nsIWBP = Ci.nsIWebBrowserPersist;
    var persist = Cc["@mozilla.org/embedding/browser/nsWebBrowserPersist;1"]
                  .createInstance(Ci.nsIWebBrowserPersist);
    persist.persistFlags = nsIWBP.PERSIST_FLAGS_REPLACE_EXISTING_FILES |
                           nsIWBP.PERSIST_FLAGS_BYPASS_CACHE |
                           nsIWBP.PERSIST_FLAGS_AUTODETECT_APPLY_CONVERSION;

    var dirSvc = Cc["@mozilla.org/file/directory_service;1"].
                 getService(Ci.nsIProperties);
    var destFile = dirSvc.get("ProfD", Ci.nsIFile);
    destFile.append("download.result");
    if (destFile.exists())
      destFile.remove(false);

    var dl = dm.addDownload(Ci.nsIDownloadManager.DOWNLOAD_TYPE_DOWNLOAD,
                            createURI("http://example.com/httpd.js"),
                            createURI(destFile), null, null,
                            Math.round(Date.now() * 1000), null, persist);

    persist.progressListener = dl.QueryInterface(Ci.nsIWebProgressListener);
    persist.saveURI(dl.source, null, null, null, null, dl.targetFile);

    return dl;
  }

  
  dm.DBConnection.executeSimpleSQL("DELETE FROM moz_downloads");

  
  var wm = Cc["@mozilla.org/appshell/window-mediator;1"].
           getService(Ci.nsIWindowMediator);
  var win = wm.getMostRecentWindow("Download:Manager");
  if (win)
    win.close();

  
  Cc["@mozilla.org/download-manager-ui;1"].
  getService(Ci.nsIDownloadManagerUI).show();

  
  function finishUp() {
    var win = wm.getMostRecentWindow("Download:Manager");

    dm.addListener(new bug413985obs(win));

    var dl = addDownload();
    
    var doc = win.document;
    doc.getElementById("downloadView").selectedIndex = 0;
  }
  
  waitForExplicitFinish();
  window.setTimeout(finishUp, 500);
}
