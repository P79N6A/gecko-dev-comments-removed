







































function cleanup()
{
  
  var rdfFile = dirSvc.get("DLoads", Ci.nsIFile);
  if (rdfFile.exists()) rdfFile.remove(true);
  
  
  var dbFile = dirSvc.get("ProfD", Ci.nsIFile);
  dbFile.append("downloads.sqlite");
  if (dbFile.exists())
    try { dbFile.remove(true); } catch(e) {  }
}

cleanup();

importDownloadsFile("bug_381535_downloads.rdf");
importDownloadsFile("bug_409179_downloads.sqlite");

function run_test()
{
  var rdfFile = dirSvc.get("DLoads", Ci.nsIFile);
  do_check_true(rdfFile.exists());

  var dm = Cc["@mozilla.org/download-manager;1"].
           getService(Ci.nsIDownloadManager);

  dm.cleanUp();
  
  do_check_false(rdfFile.exists());

  cleanup();
}
