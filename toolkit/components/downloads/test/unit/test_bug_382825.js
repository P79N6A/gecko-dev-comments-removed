






const nsIDownloadManager = Ci.nsIDownloadManager;
const dm = Cc["@mozilla.org/download-manager;1"].getService(nsIDownloadManager);

function test_retry_canceled()
{
  var dl = addDownload(httpserv);

  
  
  gDownloadCount++;

  dm.cancelDownload(dl.id);

  do_check_eq(nsIDownloadManager.DOWNLOAD_CANCELED, dl.state);

  
  dm.retryDownload(dl.id);
}

function test_retry_bad()
{
  try {
    dm.retryDownload(0);
    do_throw("Hey!  We expect to get an exception with this!");
  } catch(e) {
    do_check_eq(Components.lastResult, Cr.NS_ERROR_NOT_AVAILABLE);
  }
}

var tests = [test_retry_canceled, test_retry_bad];

var httpserv = null;
function run_test()
{
  if (oldDownloadManagerDisabled()) {
    return;
  }

  httpserv = new HttpServer();
  httpserv.registerDirectory("/", do_get_cwd());
  httpserv.start(-1);

  dm.addListener(getDownloadListener());

  for (var i = 0; i < tests.length; i++)
    tests[i]();
}
