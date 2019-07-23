










































const nsIF = Ci.nsIFile;
const nsIDM = Ci.nsIDownloadManager;
const nsIWBP = Ci.nsIWebBrowserPersist;
const nsIWPL = Ci.nsIWebProgressListener;
const dm = Cc["@mozilla.org/download-manager;1"].getService(nsIDM);

function run_test()
{
  


  
  var data = "1234567890";
  
  for (var i = 0; i < 4; i++) {
    data = [data,data,data,data,data,data,data,data,data,data,"\n"].join("");
  }

  


  var httpserv = new nsHttpServer();
  var didResumeServer = false;
  httpserv.registerPathHandler("/resume", function(meta, resp) {
    var body = data;
    resp.setHeader("Content-Type", "text/html", false);
    if (meta.hasHeader("Range")) {
      
      didResumeServer = true;
      
      var matches = meta.getHeader("Range").match(/^\s*bytes=(\d+)?-(\d+)?\s*$/);
      var from = (matches[1] === undefined) ? 0 : matches[1];
      var to = (matches[2] === undefined) ? data.length - 1 : matches[2];
      if (from >= data.length) {
        resp.setStatusLine(meta.httpVersion, 416, "Start pos too high");
        resp.setHeader("Content-Range", "*/" + data.length, false);
        return;
      }
      body = body.substring(from, to + 1);
      
      resp.setStatusLine(meta.httpVersion, 206, "Partial Content");
      resp.setHeader("Content-Range", from + "-" + to + "/" + data.length, false);
    }
    resp.bodyOutputStream.write(body, body.length);
  });
  httpserv.start(4444);

  


  var didPause = false;
  var didResumeDownload = false;
  dm.addListener({
    onDownloadStateChange: function(a, aDl) {
      if (aDl.state == nsIDM.DOWNLOAD_DOWNLOADING && !didPause) {
        


        dm.pauseDownload(aDl.id);
      } else if (aDl.state == nsIDM.DOWNLOAD_PAUSED) {
        


        didPause = true;
        
        aDl.targetFile.remove(false);
      } else if (aDl.state == nsIDM.DOWNLOAD_FINISHED) {
        


        
        do_check_true(didPause);
        
        do_check_true(didResumeDownload);
        
        do_check_true(didResumeServer);
        
        do_check_eq(data.length, aDl.targetFile.fileSize);
        
        do_check_eq(data.length, aDl.amountTransferred);
        do_check_eq(data.length, aDl.size);

        httpserv.stop(do_test_finished);
      }
    },
    onStateChange: function(a, b, aState, d, aDl) {
      if ((aState & nsIWPL.STATE_STOP) && didPause && !didResumeServer &&
          !didResumeDownload) {
        


        dm.resumeDownload(aDl.id);
        didResumeDownload = true;
      }
    },
    onProgressChange: function(a, b, c, d, e, f, g) { },
    onSecurityChange: function(a, b, c, d) { }
  });
  dm.addListener(getDownloadListener());

  


  var destFile = dirSvc.get("ProfD", nsIF);
  destFile.append("resumed");
  if (destFile.exists())
    destFile.remove(false);
  var persist = Cc["@mozilla.org/embedding/browser/nsWebBrowserPersist;1"].
                createInstance(nsIWBP);
  persist.persistFlags = nsIWBP.PERSIST_FLAGS_REPLACE_EXISTING_FILES |
                         nsIWBP.PERSIST_FLAGS_BYPASS_CACHE |
                         nsIWBP.PERSIST_FLAGS_AUTODETECT_APPLY_CONVERSION;
  var dl = dm.addDownload(nsIDM.DOWNLOAD_TYPE_DOWNLOAD,
                          createURI("http://localhost:4444/resume"),
                          createURI(destFile), null, null,
                          Math.round(Date.now() * 1000), null, persist);
  persist.progressListener = dl.QueryInterface(nsIWPL);
  persist.saveURI(dl.source, null, null, null, null, dl.targetFile);

  
  do_test_pending();
}
