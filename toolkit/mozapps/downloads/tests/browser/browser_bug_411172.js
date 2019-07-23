




































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
    state: Ci.nsIDownloadManager.DOWNLOAD_FINISHED,
    currBytes: 0, maxBytes: -1, preferredAction: 0, autoResume: 0 },
  { name: "381603.patch",
    source: "https://bugzilla.mozilla.org/attachment.cgi?id=266520",
    target: gTestPath,
    startTime: 1180493839859230,
    endTime: 1180493839859238,
    state: Ci.nsIDownloadManager.DOWNLOAD_FINISHED,
    currBytes: 0, maxBytes: -1, preferredAction: 0, autoResume: 0 },
  { name: "381603.patch",
    source: "https://bugzilla.mozilla.org/attachment.cgi?id=266520",
    target: gTestPath,
    startTime: 1180493839859230,
    endTime: 1180493839859237,
    state: Ci.nsIDownloadManager.DOWNLOAD_FAILED,
    currBytes: 0, maxBytes: -1, preferredAction: 0, autoResume: 0 },
  { name: "381603.patch",
    source: "https://bugzilla.mozilla.org/attachment.cgi?id=266520",
    target: gTestPath,
    startTime: 1180493839859230,
    endTime: 1180493839859236,
    state: Ci.nsIDownloadManager.DOWNLOAD_FAILED,
    currBytes: 0, maxBytes: -1, preferredAction: 0, autoResume: 0 },
  { name: "381603.patch",
    source: "https://bugzilla.mozilla.org/attachment.cgi?id=266520",
    target: gTestPath,
    startTime: 1180493839859230,
    endTime: 1180493839859235,
    state: Ci.nsIDownloadManager.DOWNLOAD_CANCELED,
    currBytes: 0, maxBytes: -1, preferredAction: 0, autoResume: 0 },
  { name: "381603.patch",
    source: "https://bugzilla.mozilla.org/attachment.cgi?id=266520",
    target: gTestPath,
    startTime: 1180493839859230,
    endTime: 1180493839859234,
    state: Ci.nsIDownloadManager.DOWNLOAD_CANCELED,
    currBytes: 0, maxBytes: -1, preferredAction: 0, autoResume: 0 },
  { name: "381603.patch",
    source: "https://bugzilla.mozilla.org/attachment.cgi?id=266520",
    target: gTestPath,
    startTime: 1180493839859230,
    endTime: 1180493839859233,
    state: Ci.nsIDownloadManager.DOWNLOAD_BLOCKED,
    currBytes: 0, maxBytes: -1, preferredAction: 0, autoResume: 0 },
  { name: "381603.patch",
    source: "https://bugzilla.mozilla.org/attachment.cgi?id=266520",
    target: gTestPath,
    startTime: 1180493839859230,
    endTime: 1180493839859232,
    state: Ci.nsIDownloadManager.DOWNLOAD_BLOCKED,
    currBytes: 0, maxBytes: -1, preferredAction: 0, autoResume: 0 },
  { name: "381603.patch",
    source: "https://bugzilla.mozilla.org/attachment.cgi?id=266520",
    target: gTestPath,
    startTime: 1180493839859230,
    endTime: 1180493839859231,
    state: Ci.nsIDownloadManager.DOWNLOAD_DIRTY,
    currBytes: 0, maxBytes: -1, preferredAction: 0, autoResume: 0 },
  { name: "381603.patch",
    source: "https://bugzilla.mozilla.org/attachment.cgi?id=266520",
    target: gTestPath,
    startTime: 1180493839859230,
    endTime: 1180493839859230,
    state: Ci.nsIDownloadManager.DOWNLOAD_DIRTY,
    currBytes: 0, maxBytes: -1, preferredAction: 0, autoResume: 0 }
];


function synthesizeKey(aKey, aWin)
{
  netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

  var utils = aWin.QueryInterface(Ci.nsIInterfaceRequestor)
                  .getInterface(Ci.nsIDOMWindowUtils);
  if (utils) {
    var charCode = 0;
    var keyCode = Ci.nsIDOMKeyEvent[aKey];
    var modifiers = 0;
    utils.sendKeyEvent("keydown", keyCode, charCode, modifiers);
    utils.sendKeyEvent("keypress", keyCode, charCode, modifiers);
    utils.sendKeyEvent("keyup", keyCode, charCode, modifiers);
  }
}

function test_deleteKeyRemoves(aWin)
{
  
  var doc = aWin.document;

  var dm = Cc["@mozilla.org/download-manager;1"].
           getService(Ci.nsIDownloadManager);
  var db = dm.DBConnection;
  var stmt = db.createStatement("SELECT COUNT(*) FROM moz_downloads");
  stmt.executeStep();
  var richlistbox = doc.getElementById("downloadView");
  is(stmt.getInt32(0), richlistbox.children.length,
     "The database and the number of downloads display matches");
  stmt.reset();

  var len = DownloadData.length;
  for (var i = 0; i < len; i++) {
    var key = i % 2 ? "DOM_VK_DELETE" : "DOM_VK_BACK_SPACE";
    synthesizeKey(key, aWin);

    stmt.executeStep();
    is(stmt.getInt32(0), len - (i + 1),
       "The download was properly removed");
    stmt.reset();
  }
}

var testFuncs = [
    test_deleteKeyRemoves
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
