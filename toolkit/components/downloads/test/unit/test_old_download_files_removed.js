






































function run_test()
{
  
  importDownloadsFile("empty_downloads.rdf");

  
  let rdfFile = dirSvc.get("DLoads", Ci.nsIFile);
  do_check_true(rdfFile.exists());

  
  Cc["@mozilla.org/download-manager;1"].getService(Ci.nsIDownloadManager);
  do_check_false(rdfFile.exists());
}
