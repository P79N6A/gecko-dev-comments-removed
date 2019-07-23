






































const nsIDownloadManager = Ci.nsIDownloadManager;
const dm = Cc["@mozilla.org/download-manager;1"].getService(nsIDownloadManager);

function test_get_download_empty_queue()
{
  try {
    dm.getDownload(0);
    do_throw("Hey!  We expect to get an excpetion with this!");
  } catch(e) {
    do_check_eq(Components.lastResult, Cr.NS_ERROR_NOT_AVAILABLE);
  }
}

function test_connection()
{
  print("*** DOWNLOAD MANAGER TEST - test_connection");
  var ds = dm.DBConnection;

  do_check_true(ds.connectionReady);

  do_check_true(ds.tableExists("moz_downloads"));
}

function test_count_empty_queue()
{
  print("*** DOWNLOAD MANAGER TEST - test_count_empty_queue");
  do_check_eq(0, dm.activeDownloadCount);

  do_check_false(dm.activeDownloads.hasMoreElements());
}

function test_canCleanUp_empty_queue()
{
  print("*** DOWNLOAD MANAGER TEST - test_canCleanUp_empty_queue");
  do_check_false(dm.canCleanUp);
}

function test_pauseDownload_empty_queue()
{
  print("*** DOWNLOAD MANAGER TEST - test_pauseDownload_empty_queue");
  try {
    dm.pauseDownload(0);
    do_throw("This should not be reached");
  } catch (e) {
    do_check_eq(Cr.NS_ERROR_FAILURE, e.result);
  }
}

function test_resumeDownload_empty_queue()
{
  print("*** DOWNLOAD MANAGER TEST - test_resumeDownload_empty_queue");
  try {
    dm.resumeDownload(0);
    do_throw("This should not be reached");
  } catch (e) {
    do_check_eq(Cr.NS_ERROR_FAILURE, e.result);
  }
}

function test_addDownload_normal()
{
  print("*** DOWNLOAD MANAGER TEST - Testing normal download adding");
  addDownload();
}

function test_addDownload_cancel()
{
  print("*** DOWNLOAD MANAGER TEST - Testing download cancel");
  var dl = addDownload();

  dm.cancelDownload(dl.id);

  do_check_eq(nsIDownloadManager.DOWNLOAD_CANCELED, dl.state);
}


function test_dm_getDownload(aDl)
{
  
  var dl = dm.getDownload(aDl.id);
  
  do_check_eq(aDl.displayName, dl.displayName);
}

var tests = [test_get_download_empty_queue, test_connection,
             test_count_empty_queue, test_canCleanUp_empty_queue,
             test_pauseDownload_empty_queue, test_resumeDownload_empty_queue,
             test_addDownload_normal, test_addDownload_cancel];

var httpserv = null;
function run_test()
{
  httpserv = new nsHttpServer();
  httpserv.registerDirectory("/", dirSvc.get("ProfD", Ci.nsILocalFile));
  httpserv.start(4444);

  
  var listener = {
    
    onDownloadStateChange: function(aState, aDownload)
    {
      do_check_eq(gDownloadCount, dm.activeDownloadCount);
    },
    onStateChange: function(a, b, c, d, e) { },
    onProgressChange: function(a, b, c, d, e, f, g) { },
    onStatusChange: function(a, b, c, d, e) { },
    onLocationChange: function(a, b, c, d) { },
    onSecurityChange: function(a, b, c, d) { }
  };
  dm.addListener(listener);
  dm.addListener(getDownloadListener());

  var observer = {
    observe: function(aSubject, aTopic, aData) {
      var dl = aSubject.QueryInterface(Ci.nsIDownload);
      switch (aTopic) {
        case "dl-start":
          do_check_eq(nsIDownloadManager.DOWNLOAD_DOWNLOADING, dl.state);
          do_test_pending();
          break;
        case "dl-failed":
          do_check_eq(nsIDownloadManager.DOWNLOAD_FAILED, dl.state);
          do_check_true(dm.canCleanUp);
          test_dm_getDownload(dl);
          do_test_finished();
          break;
        case "dl-cancel":
          do_check_eq(nsIDownloadManager.DOWNLOAD_CANCELED, dl.state);
          do_check_true(dm.canCleanUp);
          test_dm_getDownload(dl);
          do_test_finished();
          break;
        case "dl-done":
          test_dm_getDownload(dl);
          dm.removeDownload(dl.id);

          var stmt = dm.DBConnection.createStatement("SELECT COUNT(*) " +
                                                     "FROM moz_downloads " +
                                                     "WHERE id = ?1");
          stmt.bindInt32Parameter(0, dl.id);
          stmt.executeStep();

          do_check_eq(0, stmt.getInt32(0));
          stmt.reset();

          do_check_eq(nsIDownloadManager.DOWNLOAD_FINISHED, dl.state);
          do_check_true(dm.canCleanUp);
          do_test_finished();
          break;
      };
    }
  };
  var os = Cc["@mozilla.org/observer-service;1"]
           .getService(Ci.nsIObserverService);
  os.addObserver(observer, "dl-start", false);
  os.addObserver(observer, "dl-failed", false);
  os.addObserver(observer, "dl-cancel", false);
  os.addObserver(observer, "dl-done", false);

  for (var i = 0; i < tests.length; i++)
    tests[i]();

  cleanup();

  try {
    var thread = Cc["@mozilla.org/thread-manager;1"]
                 .getService().currentThread;

    while (!httpserv.isStopped())
      thread.processNextEvent(true);

    
    while (thread.hasPendingEvents())
      thread.processNextEvent(true);
  } catch (e) {
    print(e);
  }
}
