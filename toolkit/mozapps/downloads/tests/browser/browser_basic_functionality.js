




































var ios = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);
var dmFile = Cc["@mozilla.org/file/directory_service;1"].
             getService(Ci.nsIProperties).get("TmpD", Ci.nsIFile);
dmFile.append("dmuitest.file");
dmFile.createUnique(Ci.nsIFile.NORMAL_FILE_TYPE, 0666);
var gTestPath = ios.newFileURI(dmFile).spec;


const DownloadData = [
  
  { name: "381603.patch",
    source: "https://bugzilla.mozilla.org/attachment.cgi?id=266520",
    target: gTestPath,
    startTime: 1180493839859230,
    endTime: 1180493839859239,
    state: Ci.nsIDownloadManager.DOWNLOAD_NOTSTARTED,
    currBytes: 0, maxBytes: -1, preferredAction: 0, autoResume: 0 },
  { name: "381603.patch",
    source: "https://bugzilla.mozilla.org/attachment.cgi?id=266520",
    target: gTestPath,
    startTime: 1180493839859230,
    endTime: 1180493839859238,
    state: Ci.nsIDownloadManager.DOWNLOAD_DOWNLOADING,
    currBytes: 0, maxBytes: -1, preferredAction: 0, autoResume: 0 },
  { name: "381603.patch",
    source: "https://bugzilla.mozilla.org/attachment.cgi?id=266520",
    target: gTestPath,
    startTime: 1180493839859230,
    endTime: 1180493839859237,
    state: Ci.nsIDownloadManager.DOWNLOAD_PAUSED,
    currBytes: 0, maxBytes: -1, preferredAction: 0, autoResume: 0 },
  { name: "381603.patch",
    source: "https://bugzilla.mozilla.org/attachment.cgi?id=266520",
    target: gTestPath,
    startTime: 1180493839859230,
    endTime: 1180493839859236,
    state: Ci.nsIDownloadManager.DOWNLOAD_SCANNING,
    currBytes: 0, maxBytes: -1, preferredAction: 0, autoResume: 0 },
  { name: "381603.patch",
    source: "https://bugzilla.mozilla.org/attachment.cgi?id=266520",
    target: gTestPath,
    startTime: 1180493839859230,
    endTime: 1180493839859235,
    state: Ci.nsIDownloadManager.DOWNLOAD_QUEUED,
    currBytes: 0, maxBytes: -1, preferredAction: 0, autoResume: 0 },
  
  { name: "381603.patch",
    source: "https://bugzilla.mozilla.org/attachment.cgi?id=266520",
    target: gTestPath,
    startTime: 1180493839859230,
    endTime: 1180493839859234,
    state: Ci.nsIDownloadManager.DOWNLOAD_FINISHED,
    currBytes: 0, maxBytes: -1, preferredAction: 0, autoResume: 0 },
  { name: "381603.patch",
    source: "https://bugzilla.mozilla.org/attachment.cgi?id=266520",
    target: gTestPath,
    startTime: 1180493839859230,
    endTime: 1180493839859233,
    state: Ci.nsIDownloadManager.DOWNLOAD_FAILED,
    currBytes: 0, maxBytes: -1, preferredAction: 0, autoResume: 0 },
  { name: "381603.patch",
    source: "https://bugzilla.mozilla.org/attachment.cgi?id=266520",
    target: gTestPath,
    startTime: 1180493839859230,
    endTime: 1180493839859232,
    state: Ci.nsIDownloadManager.DOWNLOAD_CANCELED,
    currBytes: 0, maxBytes: -1, preferredAction: 0, autoResume: 0 },
  { name: "381603.patch",
    source: "https://bugzilla.mozilla.org/attachment.cgi?id=266520",
    target: gTestPath,
    startTime: 1180493839859230,
    endTime: 1180493839859231,
    state: Ci.nsIDownloadManager.DOWNLOAD_BLOCKED_PARENTAL,
    currBytes: 0, maxBytes: -1, preferredAction: 0, autoResume: 0 },
  { name: "381603.patch",
    source: "https://bugzilla.mozilla.org/attachment.cgi?id=266520",
    target: gTestPath,
    startTime: 1180493839859230,
    endTime: 1180493839859230,
    state: Ci.nsIDownloadManager.DOWNLOAD_DIRTY,
    currBytes: 0, maxBytes: -1, preferredAction: 0, autoResume: 0 },
  { name: "381603.patch",
    source: "https://bugzilla.mozilla.org/attachment.cgi?id=266520",
    target: gTestPath,
    startTime: 1180493839859229,
    endTime: 1180493839859229,
    state: Ci.nsIDownloadManager.DOWNLOAD_BLOCKED_POLICY,
    currBytes: 0, maxBytes: -1, preferredAction: 0, autoResume: 0 }
];

function test_numberOfRichlistitems(aWin)
{
  var doc = aWin.document;
  var richlistbox = doc.getElementById("downloadView");
  is(richlistbox.children.length, DownloadData.length,
     "There is the correct number of richlistitems");
}

function test_properDownloadData(aWin)
{
  
  var doc = aWin.document;
  var richlistbox = doc.getElementById("downloadView");
  for (var i = 0; i < richlistbox.children.length; i++) {
    var elm = richlistbox.children[i];
    is(elm.getAttribute("target"), DownloadData[i].name,
       "Download names match up");
    is(elm.getAttribute("state"), DownloadData[i].state,
       "Download states match up");
    is(elm.getAttribute("file"), DownloadData[i].target,
       "Download targets match up");
    is(elm.getAttribute("uri"), DownloadData[i].source,
       "Download sources match up");
  }
}

var testFuncs = [
    test_numberOfRichlistitems
  , test_properDownloadData
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
    finish();
  }
  
  waitForExplicitFinish();
  
  window.setTimeout(finishUp, 3000);
}
