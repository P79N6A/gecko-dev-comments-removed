









































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;









function getDMUI()
{
  if (Components.classesByID["{7dfdf0d1-aff6-4a34-bad1-d0fe74601642}"])
    return Components.classesByID["{7dfdf0d1-aff6-4a34-bad1-d0fe74601642}"].
           getService(Ci.nsIDownloadManagerUI);
  return false;
}








function addDownload(aName) {
    function createURI(aObj) {
      let ios = Cc["@mozilla.org/network/io-service;1"].
                getService(Ci.nsIIOService);
      return (aObj instanceof Ci.nsIFile) ? ios.newFileURI(aObj) :
                                            ios.newURI(aObj, null, null);
    }

    function randomString() {
      let chars = "ABCDEFGHIJKLMNOPQRSTUVWXTZabcdefghiklmnopqrstuvwxyz";
      let string_length = 8;
      let randomstring = "";
      for (let i=0; i<string_length; i++) {
        let rnum = Math.floor(Math.random() * chars.length);
        randomstring += chars.substring(rnum, rnum+1);
      }
      return randomstring;
    }

    let dm = Cc["@mozilla.org/download-manager;1"].
             getService(Ci.nsIDownloadManager);
    const nsIWBP = Ci.nsIWebBrowserPersist;
    let persist = Cc["@mozilla.org/embedding/browser/nsWebBrowserPersist;1"]
                  .createInstance(Ci.nsIWebBrowserPersist);
    persist.persistFlags = nsIWBP.PERSIST_FLAGS_REPLACE_EXISTING_FILES |
                           nsIWBP.PERSIST_FLAGS_BYPASS_CACHE |
                           nsIWBP.PERSIST_FLAGS_AUTODETECT_APPLY_CONVERSION;
    let dirSvc = Cc["@mozilla.org/file/directory_service;1"].
                 getService(Ci.nsIProperties);
    let dmFile = dirSvc.get("TmpD", Ci.nsIFile);
    dmFile.append(aName || ("dm-test-file-" + randomString()));
    if (dmFile.exists())
      throw "Download file already exists";

    let dl = dm.addDownload(Ci.nsIDownloadManager.DOWNLOAD_TYPE_DOWNLOAD,
                            createURI("http://example.com/httpd.js"),
                            createURI(dmFile), null, null,
                            Math.round(Date.now() * 1000), null, persist);

    persist.progressListener = dl.QueryInterface(Ci.nsIWebProgressListener);
    persist.saveURI(dl.source, null, null, null, null, dl.targetFile);

    return dl;
  }










function populateDM(DownloadData)
{
  let dm = Cc["@mozilla.org/download-manager;1"].
           getService(Ci.nsIDownloadManager);
  let db = dm.DBConnection;

  let stmt = db.createStatement(
    "INSERT INTO moz_downloads (name, source, target, startTime, endTime, " +
      "state, currBytes, maxBytes, preferredAction, autoResume) " +
    "VALUES (:name, :source, :target, :startTime, :endTime, :state, " +
      ":currBytes, :maxBytes, :preferredAction, :autoResume)");
  for each (let dl in DownloadData) {
    for (let prop in dl)
      stmt.params[prop] = dl[prop];

    stmt.execute();
  }
  stmt.finalize();
}






function getDMWindow()
{
  return Cc["@mozilla.org/appshell/window-mediator;1"].
         getService(Ci.nsIWindowMediator).
         getMostRecentWindow("Download:Manager");
}





function setCleanState()
{
  let dm = Cc["@mozilla.org/download-manager;1"].
           getService(Ci.nsIDownloadManager);

  
  dm.DBConnection.executeSimpleSQL("DELETE FROM moz_downloads");

  let win = getDMWindow();
  if (win) win.close();
}
