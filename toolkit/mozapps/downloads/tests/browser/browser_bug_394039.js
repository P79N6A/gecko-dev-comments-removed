




































var ios = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);
var dmFile = Cc["@mozilla.org/file/directory_service;1"].
             getService(Ci.nsIProperties).get("TmpD", Ci.nsIFile);
dmFile.append("dm-ui-test.file");
dmFile.createUnique(Ci.nsIFile.NORMAL_FILE_TYPE, 0666);
var gTestPath = ios.newFileURI(dmFile).spec;

const DownloadData = [
  { name: "381603.patch",
    source: "https://bugzilla.mozilla.org/attachment.cgi?id=266520",
    target: gTestPath,
    startTime: 1180493839859230,
    endTime: 1180493839859239,
    state: Ci.nsIDownloadManager.DOWNLOAD_FINISHED,
    currBytes: 0, maxBytes: -1, preferredAction: 0, autoResume: 0 }
];

function test_removeDownloadRemoves(aWin)
{
  
  var doc = aWin.document;

  var dm = Cc["@mozilla.org/download-manager;1"].
           getService(Ci.nsIDownloadManager);
  var db = dm.DBConnection;
  var stmt = db.createStatement("SELECT id FROM moz_downloads");
  stmt.executeStep();
  var id = stmt.getInt64(0);
  stmt.reset();
  stmt.finalize();

  dm.removeDownload(id);
  var richlistbox = doc.getElementById("downloadView");
  is(richlistbox.children.length, 0,
     "The download was properly removed");
}

var testFuncs = [
    test_removeDownloadRemoves
];

function test()
{
  var dm = Cc["@mozilla.org/download-manager;1"].
           getService(Ci.nsIDownloadManager);
  var db = dm.DBConnection;

  
  db.executeSimpleSQL("DELETE FROM moz_downloads");
  var rawStmt = db.createStatement(
    "INSERT INTO moz_downloads (name, source, target, startTime, endTime, " +
      "state, currBytes, maxBytes, preferredAction, autoResume) " +
    "VALUES (:name, :source, :target, :startTime, :endTime, :state, " +
      ":currBytes, :maxBytes, :preferredAction, :autoResume)");
  var stmt = Cc["@mozilla.org/storage/statement-wrapper;1"].
             createInstance(Ci.mozIStorageStatementWrapper)
  stmt.initialize(rawStmt);
  for each (var dl in DownloadData) {
    for (var prop in dl)
      stmt.params[prop] = dl[prop];
    
    stmt.execute();
  }
  stmt.statement.finalize();

  
  var wm = Cc["@mozilla.org/appshell/window-mediator;1"].
           getService(Ci.nsIWindowMediator);
  var win = wm.getMostRecentWindow("Download:Manager");
  if (win)
    win.close();

  
  Cc["@mozilla.org/download-manager-ui;1"].
  getService(Ci.nsIDownloadManagerUI).show();

  
  function finishUp() {
    var win = wm.getMostRecentWindow("Download:Manager");

    
    for each (var t in testFuncs)
      t(win);

    win.close();
    dmFile.remove(false);
    finish();
  }
  
  waitForExplicitFinish();
  
  window.setTimeout(finishUp, 3000);
}
