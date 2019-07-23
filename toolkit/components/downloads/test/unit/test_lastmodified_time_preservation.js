








































const nsIF = Ci.nsIFile;
const nsIWBP = Ci.nsIWebBrowserPersist;
const nsIWPL = Ci.nsIWebProgressListener;
const nsIDM = Ci.nsIDownloadManager;
const dm = Cc["@mozilla.org/download-manager;1"].getService(nsIDM);
const resultFileName = "test_178506" + Date.now() + ".txt";
const timeHeader = "Sun, 09 Sep 2001 01:46:40 GMT";
const timeValue = 1000000000 * 1000; 

function run_test()
{
  
  var data = "test_178506";
  var httpserv = new nsHttpServer();
  httpserv.registerPathHandler("/test_178506", function(meta, resp) {
    var body = data;
    resp.setHeader("Content-Type", "text/html", false);
    
    resp.setHeader("Last-Modified", timeHeader, false);
    resp.bodyOutputStream.write(body, body.length);
  });
  httpserv.start(4444);

  do_test_pending();

  
  var listener = {
    onDownloadStateChange: function test_178506(aState, aDownload) {
      if (aDownload.state == nsIDM.DOWNLOAD_FINISHED) {
        do_check_eq(destFile.lastModifiedTime, timeValue);
        httpserv.stop(do_test_finished);
      }
    },
    onStateChange: function(a, b, c, d, e) { },
    onProgressChange: function(a, b, c, d, e, f, g) { },
    onSecurityChange: function(a, b, c, d) { }
  };
  dm.addListener(listener);

  
  var destFile = dirSvc.get("ProfD", nsIF);
  destFile.append(resultFileName);
  if (destFile.exists())
    destFile.remove(false);

  var persist = Cc["@mozilla.org/embedding/browser/nsWebBrowserPersist;1"].
                createInstance(nsIWBP);

  var dl = dm.addDownload(nsIDM.DOWNLOAD_TYPE_DOWNLOAD,
                          createURI("http://localhost:4444/test_178506"),
                          createURI(destFile), null, null,
                          Math.round(Date.now() * 1000), null, persist);
  persist.progressListener = dl.QueryInterface(nsIWPL);
  persist.saveURI(dl.source, null, null, null, null, dl.targetFile);
}
