






































function run_test()
{
  
  importDatabaseFile("v2.sqlite");

  
  
  var dm = Cc["@mozilla.org/download-manager;1"].
           getService(Ci.nsIDownloadManager);
  var dbConn = dm.DBConnection;
  var stmt = null;

  
  do_check_true(dbConn.schemaVersion >= 3);

  
  stmt = dbConn.createStatement("SELECT referrer FROM moz_downloads");

  
  stmt = dbConn.createStatement(
    "SELECT name, source, target, startTime, endTime, state, referrer " +
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
  do_check_true(stmt.getIsNull(6));
  stmt.reset();

  cleanup();
}

