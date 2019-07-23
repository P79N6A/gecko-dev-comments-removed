







































importDownloadsFile("bug_401582_downloads.sqlite");

const nsIDownloadManager = Ci.nsIDownloadManager;
const dm = Cc["@mozilla.org/download-manager;1"].getService(nsIDownloadManager);

function test_noScanningDownloads()
{
  var stmt = dm.DBConnection.createStatement(
    "SELECT * " +
    "FROM moz_downloads " +
    "WHERE state = ?1");
  stmt.bindInt32Parameter(0, nsIDownloadManager.DOWNLOAD_SCANNING);

  do_check_false(stmt.executeStep());
  stmt.reset();
  stmt.finalize();
}

var tests = [test_noScanningDownloads];

function run_test()
{
  for (var i = 0; i < tests.length; i++)
    tests[i]();
}
