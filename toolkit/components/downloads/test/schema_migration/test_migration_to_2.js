





































function run_test()
{
  
  importDatabaseFile("v1.sqlite");

  
  
  var dm = Cc["@mozilla.org/download-manager;1"].
           getService(Ci.nsIDownloadManager);
  var dbConn = dm.DBConnection;
  var stmt = null;

  
  do_check_eq(2, dbConn.schemaVersion);

  
  try {
    
    stmt = dbConn.createStatement("SELECT iconURL FROM moz_downloads");
    do_throw("should not get here");
  } catch (e) {
    do_check_eq(Cr.NS_ERROR_FAILURE, e.result);
  }

  
  stmt = dbConn.createStatement(
    "SELECT name, source, target, startTime, endTime, state " +
    "FROM moz_downloads " +
    "WHERE id = 2");
  stmt.executeStep();
  do_check_eq("381603.patch", stmt.getString(0));
  do_check_eq("https://bugzilla.mozilla.org/attachment.cgi?id=266520",
              stmt.getUTF8String(1));
  do_check_eq("file:///Users/sdwilsh/Desktop/381603.patch",
              stmt.getUTF8String(2));
  do_check_eq(1180493839859230, stmt.getInt64(3));
  do_check_eq(1180493839859230, stmt.getInt64(4));
  do_check_eq(1, stmt.getInt32(5));
  stmt.reset();

  cleanup();
}

