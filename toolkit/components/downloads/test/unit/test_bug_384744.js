








































function run_test()
{
  var dmci = Cc["@mozilla.org/download-manager;1"].
             createInstance(Ci.nsIDownloadManager);
  var dmgs = Cc["@mozilla.org/download-manager;1"].
             getService(Ci.nsIDownloadManager);
  do_check_eq(dmci, dmgs);
}
