








const HTTP_SERVER_PORT = 4444;

function run_test()
{
  let dm = Cc["@mozilla.org/download-manager;1"].
           getService(Ci.nsIDownloadManager);
  let db = dm.DBConnection;
  var httpserv = new HttpServer();
  httpserv.start(HTTP_SERVER_PORT);

  let stmt = db.createStatement(
    "INSERT INTO moz_downloads (source, target, state, referrer) " +
    "VALUES (?1, ?2, ?3, ?4)");

  
  stmt.bindByIndex(0, "http://localhost:"+HTTP_SERVER_PORT+"/httpd.js");

  
  let file = Cc["@mozilla.org/file/directory_service;1"].
             getService(Ci.nsIProperties).get("TmpD", Ci.nsIFile);
  file.append("retry");
  file.createUnique(Ci.nsIFile.NORMAL_FILE_TYPE, 0666);
  stmt.bindByIndex(1, Cc["@mozilla.org/network/io-service;1"].
    getService(Ci.nsIIOService).newFileURI(file).spec);

  
  stmt.bindByIndex(2, dm.DOWNLOAD_CANCELED);

  
  let referrer = "http://referrer.goes/here";
  stmt.bindByIndex(3, referrer);

  
  stmt.execute();
  stmt.finalize();

  let listener = {
    onDownloadStateChange: function(aState, aDownload)
    {
      switch (aDownload.state) {
        case dm.DOWNLOAD_DOWNLOADING:
          do_check_eq(aDownload.referrer.spec, referrer);
          break;
        case dm.DOWNLOAD_FINISHED:
          do_check_eq(aDownload.referrer.spec, referrer);

          dm.removeListener(listener);
          try { file.remove(false); } catch(e) {  }
          httpserv.stop(do_test_finished);
          break;
        case dm.DOWNLOAD_FAILED:
        case dm.DOWNLOAD_CANCELED:
          httpserv.stop(function () {});
          do_throw("Unexpected download state change received, state: " +
                   aDownload.state);
          break;
      }
    }
  };
  dm.addListener(listener);

  
  dm.retryDownload(1);

  do_test_pending();
}
