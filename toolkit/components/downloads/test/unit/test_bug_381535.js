








































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

const nsIDownloadManager = Ci.nsIDownloadManager;
const dm = Cc["@mozilla.org/download-manager;1"].getService(nsIDownloadManager);

function test_count_entries()
{
  var stmt = dm.DBConnection.createStatement("SELECT COUNT(*) " +
                                             "FROM moz_downloads");
  stmt.executeStep();

  do_check_eq(1, stmt.getInt32(0));

  stmt.reset();
}

function test_download_data()
{
  var stmt = dm.DBConnection.createStatement("SELECT COUNT(*), source, target," +
                                             "iconURL, state, startTime," +
                                             "endTime " +
                                             "FROM moz_downloads " +
                                             "WHERE name = ?1");
  stmt.bindStringParameter(0, "Google.htm");
  stmt.executeStep();

  do_check_eq(1, stmt.getInt32(0));
  do_check_eq("http://www.google.com/", stmt.getUTF8String(1));
  do_check_eq("file:///Users/jruderman/Desktop/download/Google.htm", stmt.getUTF8String(2));
  do_check_eq("", stmt.getString(3));
  do_check_eq(1, stmt.getInt32(4));
  
  
  do_check_eq(stmt.getInt64(5), stmt.getInt64(6));

  stmt.reset();
}

var tests = [test_count_entries, test_download_data];

function run_test()
{
  for (var i = 0; i < tests.length; i++)
    tests[i]();
  
  cleanup();
}
