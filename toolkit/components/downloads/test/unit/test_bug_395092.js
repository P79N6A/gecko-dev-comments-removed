







































const nsIDownloadManager = Ci.nsIDownloadManager;
const dm = Cc["@mozilla.org/download-manager;1"].getService(nsIDownloadManager);

var observer = {
  mCount: 0,
  id: 0,
  observe: function observe(aSubject, aTopic, aData)
  {
    print("observering " + aTopic);
    if ("dl-start" == aTopic) {
      var dl = aSubject.QueryInterface(Ci.nsIDownload);
      this.id = dl.id;
      dm.pauseDownload(this.id);
      this.mCount++;
      do_check_eq(1, this.mCount);
    } else if ("timer-callback" == aTopic) {
      dm.resumeDownload(this.id);
    }
  }
};

var httpserv = null;
var timer = null;
function run_test()
{
  httpserv = new nsHttpServer();
  httpserv.registerDirectory("/", dirSvc.get("ProfD", Ci.nsILocalFile));
  httpserv.start(4444);

  
  var listener = {
    onDownloadStateChange: function(aOldState, aDownload)
    {
      if (Ci.nsIDownloadManager.DOWNLOAD_PAUSED == aDownload.state) {
        
        
        timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
        timer.init(observer, 0, Ci.nsITimer.TYPE_ONE_SHOT);
      }

      if (Ci.nsIDownloadManager.DOWNLOAD_FINISHED == aDownload.state)
        do_test_finished();
    },
    onStateChange: function(a, b, c, d, e) { },
    onProgressChange: function(a, b, c, d, e, f, g) { },
    onSecurityChange: function(a, b, c, d) { }
  };
  dm.addListener(listener);
  dm.addListener(getDownloadListener());

  var os = Cc["@mozilla.org/observer-service;1"].
           getService(Ci.nsIObserverService);
  os.addObserver(observer, "dl-start", false);

  addDownload();
  do_test_pending();
}
