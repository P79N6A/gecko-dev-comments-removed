









































function test()
{
  let dm = Cc["@mozilla.org/download-manager;1"].
           getService(Ci.nsIDownloadManager);
  let db = dm.DBConnection;

  
  db.executeSimpleSQL("DELETE FROM moz_downloads");

  let stmt = db.createStatement(
    "INSERT INTO moz_downloads (source, state, target, referrer) " +
    "VALUES (?1, ?2, ?3, ?4)");

  try {
    for each (let site in ["ed.agadak.net", "mozilla.org", "mozilla.com", "mozilla.net"]) {
      let file = Cc["@mozilla.org/file/directory_service;1"].
        getService(Ci.nsIProperties).get("TmpD", Ci.nsIFile);
      file.append(site);
      let fileSpec = Cc["@mozilla.org/network/io-service;1"].
        getService(Ci.nsIIOService).newFileURI(file).spec;

      stmt.bindStringParameter(0, "http://" + site + "/file");
      stmt.bindInt32Parameter(1, dm.DOWNLOAD_FINISHED);
      stmt.bindStringParameter(2, fileSpec);
      stmt.bindStringParameter(3, "http://referrer/");

      
      stmt.execute();
    }
  }
  finally {
    stmt.reset();
    stmt.finalize();
  }

  
  let wm = Cc["@mozilla.org/appshell/window-mediator;1"].
           getService(Ci.nsIWindowMediator);
  let win = wm.getMostRecentWindow("Download:Manager");
  if (win) win.close();

  let obs = Cc["@mozilla.org/observer-service;1"].
            getService(Ci.nsIObserverService);
  const DLMGR_UI_DONE = "download-manager-ui-done";

  let testPhase = 0;
  let testObs = {
    observe: function(aSubject, aTopic, aData) {
      if (aTopic != DLMGR_UI_DONE)
        return;

      let win = aSubject.QueryInterface(Ci.nsIDOMWindow);
      let $ = function(aId) win.document.getElementById(aId);
      let downloadView = $("downloadView");

      
      let invokeCount = 0;
      let counter = function() invokeCount++;

      
      let getItems = function() downloadView.itemCount;
      let getSelected = function() downloadView.selectedCount;
      let getClipboard = function() {
        let clip = Cc["@mozilla.org/widget/clipboard;1"].
                   getService(Ci.nsIClipboard);
        let trans = Cc["@mozilla.org/widget/transferable;1"].
                    createInstance(Ci.nsITransferable);
        trans.addDataFlavor("text/unicode");
        clip.getData(trans, clip.kGlobalClipboard);
        let str = {};
        let strLen = {};
        trans.getTransferData("text/unicode", str, strLen);
        return str.value.QueryInterface(Ci.nsISupportsString).data.
          substring(0, strLen.value / 2);
      };

      
      
      
      let commandTests = [
        ["pause", "pauseDownload", counter, counter],
        ["resume", "resumeDownload", counter, counter],
        ["cancel", "cancelDownload", counter, counter],
        ["open", "openDownload", counter, counter],
        ["show", "showDownload", counter, counter],
        ["retry", "retryDownload", counter, counter],
        ["openReferrer", "openReferrer", counter, counter],
        ["copyLocation", null, null, getClipboard],
        ["removeFromList", null, null, getItems],
        ["selectAll", null, null, getSelected],
      ];

      
      let allExpected = {
        single: {
          pause: [0, "Paused no downloads"],
          resume: [0, "Resumed no downloads"],
          cancel: [0, "Canceled no downloads"],
          open: [0, "Opened no downloads"],
          show: [0, "Showed no downloads"],
          retry: [1, "Retried one download"],
          openReferrer: [1, "Opened one referrer"],
          copyLocation: ["http://ed.agadak.net/file", "Copied one location"],
          removeFromList: [3, "Removed one downloads, remaining 3"],
          selectAll: [3, "Selected all 3 remaining downloads"],
        },
        double: {
          pause: [0, "Paused neither download"],
          resume: [0, "Resumed neither download"],
          cancel: [0, "Canceled neither download"],
          open: [0, "Opened neither download"],
          show: [0, "Showed neither download"],
          retry: [2, "Retried both downloads"],
          openReferrer: [2, "Opened both referrers"],
          copyLocation: ["http://mozilla.org/file\nhttp://mozilla.com/file", "Copied both locations"],
          removeFromList: [1, "Removed both downloads, remaining 1"],
          selectAll: [1, "Selected the 1 remaining download"],
        },
      };

      
      for each (let whichTest in ["single", "double"]) {
        let expected = allExpected[whichTest];

        
        downloadView.selectedIndex = 0;

        
        if (whichTest == "double")
          EventUtils.synthesizeKey("VK_DOWN", { shiftKey: true }, win);

        for each (let [command, func, test, value] in commandTests) {
          
          let copy;
          [copy, win[func]] = [win[func], test];

          
          $("menuitem_" + command).doCommand();

          
          let [correct, message] = expected[command];
          ok(value() == correct, message);

          
          invokeCount = 0;
          win[func] = copy;
        }
      }

      
      obs.removeObserver(testObs, DLMGR_UI_DONE);
      finish();
    }
  };
  obs.addObserver(testObs, DLMGR_UI_DONE, false);
 
  
  Cc["@mozilla.org/download-manager-ui;1"].
  getService(Ci.nsIDownloadManagerUI).show();

  waitForExplicitFinish();
}
