


















































Cc["@mozilla.org/moz/jssubscript-loader;1"].
  getService(Components.interfaces.mozIJSSubScriptLoader).
  loadSubScript("chrome://mochikit/content/MochiKit/packed.js");

Cc["@mozilla.org/moz/jssubscript-loader;1"].
  getService(Components.interfaces.mozIJSSubScriptLoader).
  loadSubScript("chrome://browser/content/sanitize.js");

const winWatch = Cc["@mozilla.org/embedcomp/window-watcher;1"].
                 getService(Ci.nsIWindowWatcher);
const dm = Cc["@mozilla.org/download-manager;1"].
           getService(Ci.nsIDownloadManager);
const bhist = Cc["@mozilla.org/browser/global-history;2"].
              getService(Ci.nsIBrowserHistory);
const formhist = Cc["@mozilla.org/satchel/form-history;1"].
                 getService(Ci.nsIFormHistory2);


var gAllTests = [

  


  function () {
    
    let uris = [];
    for (let i = 0; i < 30; i++) {
      uris.push(addHistoryWithMinutesAgo(i));
    }

    
    openWindow(function (aWin) {
      let wh = new WindowHelper(aWin);
      wh.selectDuration(Sanitizer.TIMESPAN_HOUR);
      wh.checkGrippy("Grippy should be at last row after selecting HOUR " +
                     "duration",
                     wh.getRowCount() - 1);

      
      let row = wh.getGrippyRow();
      while (row !== 0) {
        row--;
        wh.moveGrippyBy(-1);
        wh.checkGrippy("Grippy should be moved up one row", row);
      }
      wh.moveGrippyBy(-1);
      wh.checkGrippy("Grippy should remain at first row after trying to move " +
                     "it up",
                     0);
      while (row !== wh.getRowCount() - 1) {
        row++;
        wh.moveGrippyBy(1);
        wh.checkGrippy("Grippy should be moved down one row", row);
      }
      wh.moveGrippyBy(1);
      wh.checkGrippy("Grippy should remain at last row after trying to move " +
                     "it down",
                     wh.getRowCount() - 1);

      
      wh.checkPrefCheckbox("history", false);

      wh.cancelDialog();
      ensureHistoryClearedState(uris, false);

      
      blankSlate();
      ensureHistoryClearedState(uris, true);
    });
  },

  



  function () {
    
    let uris = [];
    for (let i = 0; i < 30; i++) {
      uris.push(addHistoryWithMinutesAgo(i));
    }
    let downloadIDs = [];
    for (let i = 0; i < 5; i++) {
      downloadIDs.push(addDownloadWithMinutesAgo(i));
    }
    
    let olderURIs = [];
    for (let i = 0; i < 5; i++) {
      olderURIs.push(addHistoryWithMinutesAgo(61 + i));
    }
    let olderDownloadIDs = [];
    for (let i = 0; i < 5; i++) {
      olderDownloadIDs.push(addDownloadWithMinutesAgo(61 + i));
    }
    let totalHistoryVisits = uris.length + olderURIs.length;

    
    openWindow(function (aWin) {
      let wh = new WindowHelper(aWin);
      wh.selectDuration(Sanitizer.TIMESPAN_HOUR);
      wh.checkGrippy("Grippy should be at proper row after selecting HOUR " +
                     "duration",
                     uris.length);

      
      
      wh.checkPrefCheckbox("history", true);
      wh.acceptDialog();
      ensureHistoryClearedState(uris, true);
      ensureDownloadsClearedState(downloadIDs, true);

      
      ensureHistoryClearedState(olderURIs, false);
      ensureDownloadsClearedState(olderDownloadIDs, false);

      
      blankSlate();
      ensureHistoryClearedState(olderURIs, true);
      ensureDownloadsClearedState(olderDownloadIDs, true);
    });
  },

  



  function () {
    
    let uris = [];
    for (let i = 0; i < 5; i++) {
      uris.push(addHistoryWithMinutesAgo(i));
    }
    let downloadIDs = [];
    for (let i = 0; i < 5; i++) {
      downloadIDs.push(addDownloadWithMinutesAgo(i));
    }
    let formEntries = [];
    for (let i = 0; i < 5; i++) {
      formEntries.push(addFormEntryWithMinutesAgo(i));
    }

    
    openWindow(function (aWin) {
      let wh = new WindowHelper(aWin);
      wh.selectDuration(Sanitizer.TIMESPAN_HOUR);
      wh.checkGrippy("Grippy should be at last row after selecting HOUR " +
                     "duration",
                     wh.getRowCount() - 1);

      
      wh.checkPrefCheckbox("history", false);
      wh.checkPrefCheckbox("formdata", true);
      wh.acceptDialog();

      
      ensureHistoryClearedState(uris, false);
      ensureDownloadsClearedState(downloadIDs, false);
      ensureFormEntriesClearedState(formEntries, true);

      
      blankSlate();
      ensureHistoryClearedState(uris, true);
      ensureDownloadsClearedState(downloadIDs, true);
    });
  },

  


  function () {
    
    let uris = [];
    uris.push(addHistoryWithMinutesAgo(10));  
    uris.push(addHistoryWithMinutesAgo(70));  
    uris.push(addHistoryWithMinutesAgo(130)); 
    uris.push(addHistoryWithMinutesAgo(250)); 

    
    openWindow(function (aWin) {
      let wh = new WindowHelper(aWin);
      wh.selectDuration(Sanitizer.TIMESPAN_EVERYTHING);
      wh.checkPrefCheckbox("history", true);
      wh.acceptDialog();
      ensureHistoryClearedState(uris, true);
    });
  }
];



var gDownloadId = 5555551;



var gCurrTest = 0;

var now_uSec = Date.now() * 1000;















function WindowHelper(aWin) {
  this.win = aWin;
}

WindowHelper.prototype = {
  


  acceptDialog: function () {
    is(this.win.document.documentElement.getButton("accept").disabled, false,
       "Dialog's OK button should not be disabled");
    this.win.document.documentElement.acceptDialog();
  },

  


  cancelDialog: function () {
    this.win.document.documentElement.cancelDialog();
  },

  








  checkGrippy: function (aMsg, aExpectedRow) {
    is(this.getGrippyRow(), aExpectedRow, aMsg);
    this.checkTreeSelection();
    this.ensureGrippyIsVisible();
  },

  








  checkPrefCheckbox: function (aPrefName, aCheckState) {
    var pref = "privacy.cpd." + aPrefName;
    var cb = this.win.document.querySelectorAll(
               "#itemList > [preference='" + pref + "']");
    is(cb.length, 1, "found checkbox for " + pref + " preference");
    if (cb[0].checked != aCheckState)
      cb[0].click();
  },

  




  checkTreeSelection: function () {
    let grippyRow = this.getGrippyRow();
    let sel = this.getTree().view.selection;
    if (grippyRow === 0) {
      is(sel.getRangeCount(), 0,
         "Grippy row is 0, so no tree selection should exist");
    }
    else {
      is(sel.getRangeCount(), 1,
         "Grippy row > 0, so only one tree selection range should exist");
      let min = {};
      let max = {};
      sel.getRangeAt(0, min, max);
      is(min.value, 0, "Tree selection should start at first row");
      is(max.value, grippyRow - 1,
         "Tree selection should end at row before grippy");
    }
  },

  



  ensureGrippyIsVisible: function () {
    let tbo = this.getTree().treeBoxObject;
    let firstVis = tbo.getFirstVisibleRow();
    let lastVis = tbo.getLastVisibleRow();
    let grippyRow = this.getGrippyRow();
    ok(firstVis <= grippyRow && grippyRow <= lastVis,
       "Grippy row should be visible; this inequality should be true: " +
       firstVis + " <= " + grippyRow + " <= " + lastVis);
  },

  


  getDurationDropdown: function () {
    return this.win.document.getElementById("sanitizeDurationChoice");
  },

  


  getGrippyRow: function () {
    return this.win.gContiguousSelectionTreeHelper.getGrippyRow();
  },

  


  getRowCount: function () {
    return this.getTree().view.rowCount;
  },

  


  getTree: function () {
    return this.win.gContiguousSelectionTreeHelper.tree;
  },

  



  isWarningPanelVisible: function () {
    return this.win.document.getElementById("durationDeck").selectedIndex == 1;
  },

  


  isTreeVisible: function () {
    return this.win.document.getElementById("durationDeck").selectedIndex == 0;
  },

  






  moveGrippyBy: function (aDelta) {
    if (aDelta === 0)
      return;
    let key = aDelta < 0 ? "UP" : "DOWN";
    let abs = Math.abs(aDelta);
    let treechildren = this.getTree().treeBoxObject.treeBody;
    for (let i = 0; i < abs; i++) {
      EventUtils.sendKey(key, treechildren);
    }
  },

  





  selectDuration: function (aDurVal) {
    this.getDurationDropdown().value = aDurVal;
    if (aDurVal === Sanitizer.TIMESPAN_EVERYTHING) {
      is(this.isTreeVisible(), false,
         "Tree should not be visible for TIMESPAN_EVERYTHING");
      is(this.isWarningPanelVisible(), true,
         "Warning panel should be visible for TIMESPAN_EVERYTHING");
    }
    else {
      is(this.isTreeVisible(), true,
         "Tree should be visible for non-TIMESPAN_EVERYTHING");
      is(this.isWarningPanelVisible(), false,
         "Warning panel should not be visible for non-TIMESPAN_EVERYTHING");
    }
  }
};







function addDownloadWithMinutesAgo(aMinutesAgo) {
  let name = "fakefile-" + aMinutesAgo + "-minutes-ago";
  let data = {
    id:        gDownloadId,
    name:      name,
    source:   "https://bugzilla.mozilla.org/show_bug.cgi?id=480169",
    target:    name,
    startTime: now_uSec - (aMinutesAgo * 60 * 1000000),
    endTime:   now_uSec - ((aMinutesAgo + 1) *60 * 1000000),
    state:     Ci.nsIDownloadManager.DOWNLOAD_FINISHED,
    currBytes: 0, maxBytes: -1, preferredAction: 0, autoResume: 0
  };

  let db = dm.DBConnection;
  let stmt = db.createStatement(
    "INSERT INTO moz_downloads (id, name, source, target, startTime, endTime, " +
      "state, currBytes, maxBytes, preferredAction, autoResume) " +
    "VALUES (:id, :name, :source, :target, :startTime, :endTime, :state, " +
      ":currBytes, :maxBytes, :preferredAction, :autoResume)");
  try {
    for (let prop in data) {
      stmt.params[prop] = data[prop];
    }
    stmt.execute();
  }
  finally {
    stmt.reset();
  }

  is(downloadExists(gDownloadId), true,
     "Sanity check: download " + gDownloadId +
     " should exist after creating it");

  return gDownloadId++;
}







function addFormEntryWithMinutesAgo(aMinutesAgo) {
  let name = aMinutesAgo + "-minutes-ago";
  formhist.addEntry(name, "dummy");

  
  let db = formhist.DBConnection;
  let timestamp = now_uSec - (aMinutesAgo * 60 * 1000000);
  db.executeSimpleSQL("UPDATE moz_formhistory SET firstUsed = " +
                      timestamp +  " WHERE fieldname = '" + name + "'");

  is(formhist.nameExists(name), true,
     "Sanity check: form entry " + name + " should exist after creating it");
  return name;
}







function addHistoryWithMinutesAgo(aMinutesAgo) {
  let pURI = makeURI("http://" + aMinutesAgo + "-minutes-ago.com/");
  bhist.addPageWithDetails(pURI,
                           aMinutesAgo + " minutes ago",
                           now_uSec - (aMinutesAgo * 60 * 1000 * 1000));
  is(bhist.isVisited(pURI), true,
     "Sanity check: history visit " + pURI.spec +
     " should exist after creating it");
  return pURI;
}




function blankSlate() {
  bhist.removeAllPages();
  dm.cleanUp();
  formhist.removeAllEntries();
}








function downloadExists(aID)
{
  let db = dm.DBConnection;
  let stmt = db.createStatement(
    "SELECT * " +
    "FROM moz_downloads " +
    "WHERE id = :id"
  );
  stmt.params.id = aID;
  let rows = stmt.executeStep();
  stmt.finalize();
  return !!rows;
}





function doNextTest() {
  if (gAllTests.length <= gCurrTest) {
    blankSlate();
    finish();
  }
  else {
    let ct = gCurrTest;
    gCurrTest++;
    gAllTests[ct]();
  }
}









function ensureDownloadsClearedState(aDownloadIDs, aShouldBeCleared) {
  let niceStr = aShouldBeCleared ? "no longer" : "still";
  aDownloadIDs.forEach(function (id) {
    is(downloadExists(id), !aShouldBeCleared,
       "download " + id + " should " + niceStr + " exist");
  });
}









function ensureFormEntriesClearedState(aFormEntries, aShouldBeCleared) {
  let niceStr = aShouldBeCleared ? "no longer" : "still";
  aFormEntries.forEach(function (entry) {
    is(formhist.nameExists(entry), !aShouldBeCleared,
       "form entry " + entry + " should " + niceStr + " exist");
  });
}









function ensureHistoryClearedState(aURIs, aShouldBeCleared) {
  let niceStr = aShouldBeCleared ? "no longer" : "still";
  aURIs.forEach(function (aURI) {
    is(bhist.isVisited(aURI), !aShouldBeCleared,
       "history visit " + aURI.spec + " should " + niceStr + " exist");
  });
}







function openWindow(aOnloadCallback) {
  let windowObserver = {
    observe: function(aSubject, aTopic, aData) {
      if (aTopic === "domwindowopened") {
        winWatch.unregisterNotification(this);
        let win = aSubject.QueryInterface(Ci.nsIDOMWindow);
        win.addEventListener("load", function onload(event) {
          win.removeEventListener("load", onload, false);
          executeSoon(function () {
            
            
            try {
              aOnloadCallback(win);
              doNextTest();
            }
            catch (exc) {
              win.close();
              ok(false, "Unexpected exception: " + exc + "\n" + exc.stack);
              finish();
            }
          });
        }, false);
      }
    }
  };
  winWatch.registerNotification(windowObserver);
  winWatch.openWindow(null,
                      "chrome://browser/content/sanitize.xul",
                      "Sanitize",
                      "chrome,titlebar,dialog,centerscreen,modal",
                      null);
}



function test() {
  blankSlate();
  waitForExplicitFinish();
  
  doNextTest();
}
