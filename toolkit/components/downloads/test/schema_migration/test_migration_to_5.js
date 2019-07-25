








































function run_test()
{
  
  importDatabaseFile("v4.sqlite");

  
  
  var dm = Cc["@mozilla.org/download-manager;1"].
           getService(Ci.nsIDownloadManager);
  var dbConn = dm.DBConnection;

  
  do_check_true(dbConn.schemaVersion >= 5);

  
  var stmt = dbConn.createStatement(
    "SELECT name, source, target, startTime, endTime, state, referrer, " +
           "entityID, tempPath " +
    "FROM moz_downloads " +
    "WHERE id = 27");
  stmt.executeStep();
  do_check_eq("Firefox 2.0.0.6.dmg", stmt.getString(0));
  do_check_eq("http://ftp-mozilla.netscape.com/pub/mozilla.org/firefox/releases/2.0.0.6/mac/en-US/Firefox%202.0.0.6.dmg",
              stmt.getUTF8String(1));
  do_check_eq("file:///Users/sdwilsh/Desktop/Firefox%202.0.0.6.dmg",
              stmt.getUTF8String(2));
  do_check_eq(1187390974170783, stmt.getInt64(3));
  do_check_eq(1187391001257446, stmt.getInt64(4));
  do_check_eq(1, stmt.getInt32(5));
  do_check_eq("http://www.mozilla.com/en-US/products/download.html?product=firefox-2.0.0.6&os=osx&lang=en-US",stmt.getUTF8String(6));
  do_check_true(stmt.getIsNull(7));
  do_check_true(stmt.getIsNull(8));
  stmt.finalize();

  cleanup();
}
