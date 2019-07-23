













































let doNotError = false;

const nsIF = Ci.nsIFile;
const nsIDM = Ci.nsIDownloadManager;
const nsIWBP = Ci.nsIWebBrowserPersist;
const nsIWPL = Ci.nsIWebProgressListener;
const dm = Cc["@mozilla.org/download-manager;1"].getService(nsIDM);
dm.cleanUp();

function setOnlineState(aOnline)
{
  
  
  
  let topic = aOnline ?
    "network:offline-status-changed" :
    "network:offline-about-to-go-offline";
  let state = aOnline ? "online" : "offline";
  Cc["@mozilla.org/observer-service;1"].
  getService(Ci.nsIObserverService).
  notifyObservers(null, topic, state);
}

function run_test()
{
  


  
  let data = "1234567890";
  
  for (let i = 0; i < 4; i++)
    data = [data,data,data,data,data,data,data,data,data,data,"\n"].join("");

  


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
        dump("Returning early - from >= data.length.  Not an error (bug 431745)\n");
        doNotError = true;
        return;
      }
      body = body.substring(from, to + 1);
      
      resp.setStatusLine(meta.httpVersion, 206, "Partial Content");
      resp.setHeader("Content-Range", from + "-" + to + "/" + data.length);
    }
    resp.bodyOutputStream.write(body, body.length);
  });
  httpserv.start(4444);

  


  let didPause = false;
  let didResumeDownload = false;
  dm.addListener({
    onDownloadStateChange: function(a, aDl) {
      if (aDl.state == nsIDM.DOWNLOAD_DOWNLOADING && !didPause) {
        


        setOnlineState(false);
      } else if (aDl.state == nsIDM.DOWNLOAD_PAUSED) {
        


        didPause = true;
      } else if (aDl.state == nsIDM.DOWNLOAD_FINISHED) {
        


        
        do_check_true(didPause);
        
        do_check_true(didResumeDownload);
        
        do_check_true(didResumeServer);

        httpserv.stop(do_test_finished);
        aDl.targetFile.remove(false);
      }
      else if (aDl.state == nsIDM.DOWNLOAD_FAILED) {
        
        do_check_true(doNotError);
        httpserv.stop(do_test_finished);
      }
    },
    onStateChange: function(a, b, aState, d, aDl) {
      if ((aState & nsIWPL.STATE_STOP) && didPause && !didResumeServer &&
          !didResumeDownload) {
        


        setOnlineState(true);
        didResumeDownload = true;
      }
    },
    onProgressChange: function(a, b, c, d, e, f, g) { },
    onSecurityChange: function(a, b, c, d) { }
  });
  dm.addListener(getDownloadListener());

  


  let destFile = dirSvc.get("ProfD", nsIF);
  destFile.append("offline_online");
  if (destFile.exists())
    destFile.remove(false);
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
}
