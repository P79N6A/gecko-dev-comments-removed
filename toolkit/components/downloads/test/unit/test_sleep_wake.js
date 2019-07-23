








































const nsIF = Ci.nsIFile;
const nsIDM = Ci.nsIDownloadManager;
const nsIWBP = Ci.nsIWebBrowserPersist;
const nsIWPL = Ci.nsIWebProgressListener;
const dm = Cc["@mozilla.org/download-manager;1"].getService(nsIDM);
dm.cleanUp();

function notify(aTopic)
{
  Cc["@mozilla.org/observer-service;1"].
  getService(Ci.nsIObserverService).
  notifyObservers(null, aTopic, null);
}

function run_test()
{
  


  Cc["@mozilla.org/preferences-service;1"].
  getService(Ci.nsIPrefBranch).
  setIntPref("browser.download.manager.resumeOnWakeDelay", 1000);
dump("%%%Set pref\n");

  


  
  let data = "1234567890";
  
  for (let i = 0; i < 4; i++)
    data = [data,data,data,data,data,data,data,data,data,data,"\n"].join("");
dump("%%%Generated data\n");

  


  let httpserv = new nsHttpServer();
  let didResumeServer = false;
  httpserv.registerPathHandler("/resume", function(meta, resp) {
    let body = data;
    resp.setHeader("Content-Type", "text/html", false);
    if (meta.hasHeader("Range")) {
      
      didResumeServer = true;
      
      let matches = meta.getHeader("Range").match(/^\s*bytes=(\d+)?-(\d+)?\s*$/);
      let from = (matches[1] === undefined) ? 0 : matches[1];
      let to = (matches[2] === undefined) ? data.length - 1 : matches[2];
      if (from >= data.length) {
        resp.setStatusLine(meta.httpVersion, 416, "Start pos too high");
        resp.setHeader("Content-Range", "*/" + data.length);
dump("%%% Returning early - from >= data.length\n");
        return;
      }
      body = body.substring(from, to + 1);
      
      resp.setStatusLine(meta.httpVersion, 206, "Partial Content");
      resp.setHeader("Content-Range", from + "-" + to + "/" + data.length);
    }
    resp.bodyOutputStream.write(body, body.length);
  });
  httpserv.start(4444);
dump("%%%Started server\n");

  


  let didPause = false;
  let didResumeDownload = false;
  dm.addListener({
    onDownloadStateChange: function(a, aDl) {
dump("%%%onDownloadStateChange\n");
      if (aDl.state == nsIDM.DOWNLOAD_DOWNLOADING && !didPause) {
dump("%%%aDl.state: DOWNLOAD_DOWNLOADING\n");
        


        notify("sleep_notification");
      } else if (aDl.state == nsIDM.DOWNLOAD_PAUSED) {
dump("%%%aDl.state: DOWNLOAD_PAUSED\n");
        


        didPause = true;
      } else if (aDl.state == nsIDM.DOWNLOAD_FINISHED) {
dump("%%%aDl.state: DOWNLOAD_FINISHED\n");
        


        
        do_check_true(didPause);
        
        do_check_true(didResumeDownload);
        
        do_check_true(didResumeServer);

        httpserv.stop();
        aDl.targetFile.remove(false);
        
        do_test_finished();
      } else
        dump("%%%aDl.state: " + aDl.state + "\n");
    },
    onStateChange: function(a, b, aState, d, aDl) {
dump("%%%onStateChange\n");
dump("%%%aState: " + aState + "\n");
dump("%%%status: " + d + "\n");
      if ((aState & nsIWPL.STATE_STOP) && didPause && !didResumeServer &&
          !didResumeDownload) {
        


        notify("wake_notification");
        didResumeDownload = true;
      }
    },
    onProgressChange: function(a, b, c, d, e, f, g) { },
    onSecurityChange: function(a, b, c, d) { }
  });
  dm.addListener(getDownloadListener());
dump("%%%Added listener\n");

  


  let destFile = dirSvc.get("ProfD", nsIF);
  destFile.append("sleep_wake");
  let persist = Cc["@mozilla.org/embedding/browser/nsWebBrowserPersist;1"].
                createInstance(nsIWBP);
  persist.persistFlags = nsIWBP.PERSIST_FLAGS_REPLACE_EXISTING_FILES |
                         nsIWBP.PERSIST_FLAGS_BYPASS_CACHE |
                         nsIWBP.PERSIST_FLAGS_AUTODETECT_APPLY_CONVERSION;
  let dl = dm.addDownload(nsIDM.DOWNLOAD_TYPE_DOWNLOAD,
                          createURI("http://localhost:4444/resume"),
                          createURI(destFile), null, null,
                          Math.round(Date.now() * 1000), null, persist);
  persist.progressListener = dl.QueryInterface(nsIWPL);
  persist.saveURI(dl.source, null, null, null, null, dl.targetFile);

  
  do_test_pending();
dump("%%%Started test\n");
}
