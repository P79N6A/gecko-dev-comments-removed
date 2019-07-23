



































function run_test()
{
  
  var targetVersion = 7;

  
  importDatabaseFile("v" + (targetVersion - 1) + ".sqlite");

  
  var dm = Cc["@mozilla.org/download-manager;1"].
           getService(Ci.nsIDownloadManager);
  var dbConn = dm.DBConnection;

  
  do_check_true(dbConn.schemaVersion >= targetVersion);

  
  var stmt = dbConn.createStatement(
    "SELECT name, source, target, tempPath, startTime, endTime, state, " +
           "referrer, entityID, currBytes, maxBytes, mimeType, " +
           "preferredApplication, preferredAction " +
    "FROM moz_downloads " +
    "WHERE id = 28");
  stmt.executeStep();

  
  var data = [
    "firefox-3.0a9pre.en-US.linux-i686.tar.bz2",
    "http://ftp.mozilla.org/pub/mozilla.org/firefox/nightly/latest-trunk/firefox-3.0a9pre.en-US.linux-i686.tar.bz2",
    "file:///Users/Ed/Desktop/firefox-3.0a9pre.en-US.linux-i686.tar.bz2",
    "/Users/Ed/Desktop/+EZWafFQ.bz2.part",
    1192469856209164,
    1192469877017396,
    Ci.nsIDownloadManager.DOWNLOAD_FINISHED,
    "http://ftp.mozilla.org/pub/mozilla.org/firefox/nightly/latest-trunk/",
    "%2210e66c1-8a2d6b-9b33f380%22/9055595/Mon, 15 Oct 2007 11:45:34 GMT",
    1210772,
    9055595,
    
    true,
    true,
    0,
  ];

  
  var i = 0;
  do_check_eq(data[i], stmt.getString(i++));
  do_check_eq(data[i], stmt.getUTF8String(i++));
  do_check_eq(data[i], stmt.getUTF8String(i++));
  do_check_eq(data[i], stmt.getString(i++));
  do_check_eq(data[i], stmt.getInt64(i++));
  do_check_eq(data[i], stmt.getInt64(i++));
  do_check_eq(data[i], stmt.getInt32(i++));
  do_check_eq(data[i], stmt.getUTF8String(i++));
  do_check_eq(data[i], stmt.getUTF8String(i++));
  do_check_eq(data[i], stmt.getInt64(i++));
  do_check_eq(data[i], stmt.getInt64(i++));
  do_check_eq(data[i], stmt.getIsNull(i++));
  do_check_eq(data[i], stmt.getIsNull(i++));
  do_check_eq(data[i], stmt.getInt32(i++));

  stmt.reset();
  stmt.finalize();

  cleanup();
}
