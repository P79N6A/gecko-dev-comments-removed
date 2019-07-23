




































var ios = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);
var dmFile = Cc["@mozilla.org/file/directory_service;1"].
             getService(Ci.nsIProperties).get("TmpD", Ci.nsIFile);
dmFile.append("dm-ui-test.file");
dmFile.createUnique(Ci.nsIFile.NORMAL_FILE_TYPE, 0666);
var gTestPath = ios.newFileURI(dmFile).spec;

const DownloadData = [
  { name: "Firefox 2.0.0.11.dmg",
    source: "http://mozilla-mirror.naist.jp//firefox/releases/2.0.0.11/mac/en-US/Firefox%202.0.0.11.dmg",
    target: gTestPath,
    startTime: 1200185939538521,
    endTime: 1200185939538521,
    entityID: "%22216c12-1116bd8-440070d5d2700%22/17918936/Thu, 29 Nov 2007 01:15:40 GMT",
    state: Ci.nsIDownloadManager.DOWNLOAD_DOWNLOADING,
    currBytes: 0, maxBytes: -1, preferredAction: 0, autoResume: 0 },
  { name: "Firefox 2.0.0.11.dmg",
    source: "http://mozilla-mirror.naist.jp//firefox/releases/2.0.0.11/mac/en-US/Firefox%202.0.0.11.dmg",
    target: gTestPath,
    startTime: 1200185939538520,
    endTime: 1200185939538520,
    entityID: "",
    state: Ci.nsIDownloadManager.DOWNLOAD_DOWNLOADING,
    currBytes: 0, maxBytes: -1, preferredAction: 0, autoResume: 0 }
];

function test_pauseButtonCorrect(aWin)
{
  
  var doc = aWin.document;

  var dm = Cc["@mozilla.org/download-manager;1"].
           getService(Ci.nsIDownloadManager);

  var richlistbox = doc.getElementById("downloadView");
  for (var i = 0; i < DownloadData.length; i++) {
    var dl = richlistbox.children[i];
    var buttons = dl.buttons;
    for (var j = 0; j < buttons.length; j++) {
      var button = buttons[j];
      if ("cmd_pause" == button.getAttribute("cmd")) {
        var id = dl.getAttribute("dlid");

        
        var resumable = dm.getDownload(id).resumable;
        is(DownloadData[i].entityID ? true : false, resumable,
           "Pause button is properly disabled");

        
        if (!resumable) {
          var sb = doc.getElementById("downloadStrings");
          is(button.getAttribute("tooltiptext"), sb.getString("cannotPause"),
             "Pause button has proper text");
        }
      }
    }
  }
}

var testFuncs = [
    test_pauseButtonCorrect
];

function test()
{
  var dm = Cc["@mozilla.org/download-manager;1"].
           getService(Ci.nsIDownloadManager);
  var db = dm.DBConnection;

  
  db.executeSimpleSQL("DELETE FROM moz_downloads");
  var rawStmt = db.createStatement(
    "INSERT INTO moz_downloads (name, source, target, startTime, endTime, " +
      "state, currBytes, maxBytes, preferredAction, autoResume, entityID) " +
    "VALUES (:name, :source, :target, :startTime, :endTime, :state, " +
      ":currBytes, :maxBytes, :preferredAction, :autoResume, :entityID)");
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
  
  window.setTimeout(finishUp, 2000);
}
