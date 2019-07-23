





































function test()
{
  
  

  function test_checkedAndDisabledAtStart(aWin)
  {
    let doc = aWin.document;
    let downloads = doc.getElementById("downloads-checkbox");
    let history = doc.getElementById("history-checkbox");

    ok(history.checked, "history checkbox is checked");
    ok(downloads.disabled, "downloads checkbox is disabled");
    ok(downloads.checked, "downloads checkbox is checked");
  }

  function test_checkedAndDisabledOnHistoryToggle(aWin)
  {
    let doc = aWin.document;
    let downloads = doc.getElementById("downloads-checkbox");
    let history = doc.getElementById("history-checkbox");

    EventUtils.synthesizeMouse(history, 0, 0, {}, aWin);
    ok(!history.checked, "history checkbox is not checked");
    ok(downloads.disabled, "downloads checkbox is disabled");
    ok(downloads.checked, "downloads checkbox is checked");
  }

  function test_checkedAfterAddingDownload(aWin)
  {
    let doc = aWin.document;
    let downloads = doc.getElementById("downloads-checkbox");
    let history = doc.getElementById("history-checkbox");

    
    let ios = Cc["@mozilla.org/network/io-service;1"].
              getService(Ci.nsIIOService);
    let file = Cc["@mozilla.org/file/directory_service;1"].
               getService(Ci.nsIProperties).get("TmpD", Ci.nsIFile);
    file.append("satitize-dm-test.file");
    file.createUnique(Ci.nsIFile.NORMAL_FILE_TYPE, 0666);
    let testPath = ios.newFileURI(file).spec;
    let data = {
      name: "381603.patch",
      source: "https://bugzilla.mozilla.org/attachment.cgi?id=266520",
      target: testPath,
      startTime: 1180493839859230,
      endTime: 1180493839859239,
      state: Ci.nsIDownloadManager.DOWNLOAD_FINISHED,
      currBytes: 0, maxBytes: -1, preferredAction: 0, autoResume: 0
    };
    let db = Cc["@mozilla.org/download-manager;1"].
             getService(Ci.nsIDownloadManager).DBConnection;
    let stmt = db.createStatement(
      "INSERT INTO moz_downloads (name, source, target, startTime, endTime, " +
        "state, currBytes, maxBytes, preferredAction, autoResume) " +
      "VALUES (:name, :source, :target, :startTime, :endTime, :state, " +
        ":currBytes, :maxBytes, :preferredAction, :autoResume)");
    try {
      for (let prop in data)
        stmt.params[prop] = data[prop];
      stmt.execute();
    }
    finally {
      stmt.finalize();
    }

    
    EventUtils.synthesizeMouse(history, 0, 0, {}, aWin);
    EventUtils.synthesizeMouse(history, 0, 0, {}, aWin);

    ok(!history.checked, "history checkbox is not checked");
    ok(!downloads.disabled, "downloads checkbox is not disabled");
    ok(downloads.checked, "downloads checkbox is checked");
  }

  function test_checkedAndDisabledWithHistoryChecked(aWin)
  {
    let doc = aWin.document;
    let downloads = doc.getElementById("downloads-checkbox");
    let history = doc.getElementById("history-checkbox");

    EventUtils.synthesizeMouse(history, 0, 0, {}, aWin);
    ok(history.checked, "history checkbox is checked");
    ok(downloads.disabled, "downloads checkbox is disabled");
    ok(downloads.checked, "downloads checkbox is checked");
  }

  let tests = [
    test_checkedAndDisabledAtStart,
    test_checkedAndDisabledOnHistoryToggle,
    test_checkedAfterAddingDownload,
    test_checkedAndDisabledWithHistoryChecked,
  ];

  
  

  let dm = Cc["@mozilla.org/download-manager;1"].
           getService(Ci.nsIDownloadManager);
  let db = dm.DBConnection;

  
  db.executeSimpleSQL("DELETE FROM moz_downloads");

  
  let ww = Cc["@mozilla.org/embedcomp/window-watcher;1"].
           getService(Ci.nsIWindowWatcher);
  let win = ww.getWindowByName("Sanatize", null);
  if (win && (win instanceof Ci.nsIDOMWindowInternal)) win.close();

  
  ww.registerNotification({
    observe: function(aSubject, aTopic, aData) {
      ww.unregisterNotification(this);
      aSubject.QueryInterface(Ci.nsIDOMEventTarget).
      addEventListener("DOMContentLoaded", doTest, false);
    }
  });

  
  let doTest = function() setTimeout(function() {
    let win = ww.getWindowByName("Sanitize", null)
                .QueryInterface(Ci.nsIDOMWindowInternal);

    for (let i = 0; i < tests.length; i++)
      tests[i](win);

    win.close();
    finish();
  }, 0);
 
  
  ww.openWindow(window,
                "chrome://browser/content/sanitize.xul",
                "Sanitize",
                "chrome,titlebar,centerscreen",
                null);

  waitForExplicitFinish();
}
