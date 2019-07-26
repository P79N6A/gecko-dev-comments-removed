






importDownloadsFile("bug_409179_downloads.sqlite");

function run_test()
{
  if (oldDownloadManagerDisabled()) {
    return;
  }

  var caughtException = false;
  try {
    var dm = Cc["@mozilla.org/download-manager;1"].
             getService(Ci.nsIDownloadManager);
  } catch (e) {
    caughtException = true;
  }
  do_check_false(caughtException);
}
