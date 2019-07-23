



















































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
    let wh = new WindowHelper();
    wh.onload = function () {
      
      this.selectDuration(Sanitizer.TIMESPAN_HOUR);
      
      if (!this.getItemList().collapsed)
        this.toggleDetails();
      this.acceptDialog();
    };
    wh.open();
  },

  


  function () {
    
    let uris = [];
    for (let i = 0; i < 30; i++) {
      uris.push(addHistoryWithMinutesAgo(i));
    }

    let wh = new WindowHelper();
    wh.onload = function () {
      this.selectDuration(Sanitizer.TIMESPAN_HOUR);
      this.checkPrefCheckbox("history", false);
      this.checkDetails(false);

      
      this.toggleDetails();
      this.checkDetails(true);

      
      this.toggleDetails();
      this.checkDetails(false);
      this.cancelDialog();

      ensureHistoryClearedState(uris, false);
      blankSlate();
      ensureHistoryClearedState(uris, true);
    };
    wh.open();
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

    let wh = new WindowHelper();
    wh.onload = function () {
      this.selectDuration(Sanitizer.TIMESPAN_HOUR);
      this.checkPrefCheckbox("history", true);
      this.acceptDialog();

      intPrefIs("sanitize.timeSpan", Sanitizer.TIMESPAN_HOUR,
                "timeSpan pref should be hour after accepting dialog with " +
                "hour selected");
      boolPrefIs("cpd.history", true,
                 "history pref should be true after accepting dialog with " +
                 "history checkbox checked");
      boolPrefIs("cpd.downloads", true,
                 "downloads pref should be true after accepting dialog with " +
                 "history checkbox checked");

      
      ensureHistoryClearedState(uris, true);
      ensureDownloadsClearedState(downloadIDs, true);

      
      ensureHistoryClearedState(olderURIs, false);
      ensureDownloadsClearedState(olderDownloadIDs, false);

      
      blankSlate();
      ensureHistoryClearedState(olderURIs, true);
      ensureDownloadsClearedState(olderDownloadIDs, true);
    };
    wh.open();
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

    let wh = new WindowHelper();
    wh.onload = function () {
      is(this.isWarningPanelVisible(), false,
         "Warning panel should be hidden after previously accepting dialog " +
         "with a predefined timespan");
      this.selectDuration(Sanitizer.TIMESPAN_HOUR);

      
      this.checkPrefCheckbox("history", false);
      this.checkPrefCheckbox("formdata", true);
      this.acceptDialog();

      intPrefIs("sanitize.timeSpan", Sanitizer.TIMESPAN_HOUR,
                "timeSpan pref should be hour after accepting dialog with " +
                "hour selected");
      boolPrefIs("cpd.history", false,
                 "history pref should be false after accepting dialog with " +
                 "history checkbox unchecked");
      boolPrefIs("cpd.downloads", false,
                 "downloads pref should be false after accepting dialog with " +
                 "history checkbox unchecked");

      
      ensureHistoryClearedState(uris, false);
      ensureDownloadsClearedState(downloadIDs, false);
      ensureFormEntriesClearedState(formEntries, true);

      
      blankSlate();
      ensureHistoryClearedState(uris, true);
      ensureDownloadsClearedState(downloadIDs, true);
    };
    wh.open();
  },

  


  function () {
    
    let uris = [];
    uris.push(addHistoryWithMinutesAgo(10));  
    uris.push(addHistoryWithMinutesAgo(70));  
    uris.push(addHistoryWithMinutesAgo(130)); 
    uris.push(addHistoryWithMinutesAgo(250)); 

    let wh = new WindowHelper();
    wh.onload = function () {
      is(this.isWarningPanelVisible(), false,
         "Warning panel should be hidden after previously accepting dialog " +
         "with a predefined timespan");
      this.selectDuration(Sanitizer.TIMESPAN_EVERYTHING);
      this.checkPrefCheckbox("history", true);
      this.checkDetails(true);

      
      this.toggleDetails();
      this.checkDetails(false);

      
      this.toggleDetails();
      this.checkDetails(true);

      this.acceptDialog();

      intPrefIs("sanitize.timeSpan", Sanitizer.TIMESPAN_EVERYTHING,
                "timeSpan pref should be everything after accepting dialog " +
                "with everything selected");
      ensureHistoryClearedState(uris, true);
    };
    wh.open();
  },

  



  function () {
    
    let uris = [];
    uris.push(addHistoryWithMinutesAgo(10));  
    uris.push(addHistoryWithMinutesAgo(70));  
    uris.push(addHistoryWithMinutesAgo(130)); 
    uris.push(addHistoryWithMinutesAgo(250)); 

    let wh = new WindowHelper();
    wh.onload = function () {
      is(this.isWarningPanelVisible(), true,
         "Warning panel should be visible after previously accepting dialog " +
         "with clearing everything");
      this.selectDuration(Sanitizer.TIMESPAN_EVERYTHING);
      this.checkPrefCheckbox("history", true);
      this.acceptDialog();

      intPrefIs("sanitize.timeSpan", Sanitizer.TIMESPAN_EVERYTHING,
                "timeSpan pref should be everything after accepting dialog " +
                "with everything selected");
      ensureHistoryClearedState(uris, true);
    };
    wh.open();
  },

  



  function () {
    let wh = new WindowHelper();
    wh.onload = function () {
      
      this.resetCheckboxes();
      this.selectDuration(Sanitizer.TIMESPAN_EVERYTHING);

      
      this.toggleDetails();
      this.checkDetails(false);
      this.acceptDialog();
    };
    wh.open();
  },
  function () {
    let wh = new WindowHelper();
    wh.onload = function () {
      
      
      this.checkDetails(false);

      
      this.checkPrefCheckbox("history", false);
      this.acceptDialog();
    };
    wh.open();
  },
  function () {
    let wh = new WindowHelper();
    wh.onload = function () {
      
      
      this.checkDetails(true);

      
      this.toggleDetails();
      this.checkDetails(false);
      this.cancelDialog();
    };
    wh.open();
  },
  function () {
    let wh = new WindowHelper();
    wh.onload = function () {
      
      
      this.checkDetails(true);

      
      this.selectDuration(Sanitizer.TIMESPAN_HOUR);
      
      this.toggleDetails();
      this.checkDetails(false);
      this.acceptDialog();
    };
    wh.open();
  },
  function () {
    let wh = new WindowHelper();
    wh.onload = function () {
      
      this.checkDetails(false);

      this.cancelDialog();
    };
    wh.open();
  },
  function () {
    let wh = new WindowHelper();
    wh.onload = function () {
      
      this.checkDetails(false);

      
      this.toggleDetails();
      this.checkDetails(true);
      this.cancelDialog();
    };
    wh.open();
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

  







  checkDetails: function (aShouldBeShown) {
    let button = this.getDetailsButton();
    let list = this.getItemList();
    let hidden = list.hidden || list.collapsed;
    is(hidden, !aShouldBeShown,
       "Details should be " + (aShouldBeShown ? "shown" : "hidden") +
       " but were actually " + (hidden ? "hidden" : "shown"));
    let dir = hidden ? "down" : "up";
    is(button.className, "expander-" + dir,
       "Details button should be " + dir + " because item list is " +
       (hidden ? "" : "not ") + "hidden");
    let height = 0;
    if (!hidden)
      height += list.boxObject.height;
    if (this.isWarningPanelVisible())
      height += this.getWarningPanel().boxObject.height;
    ok(height < this.win.innerHeight,
       "Window should be tall enough to fit warning panel and item list");
  },

  








  checkPrefCheckbox: function (aPrefName, aCheckState) {
    var pref = "privacy.cpd." + aPrefName;
    var cb = this.win.document.querySelectorAll(
               "#itemList > [preference='" + pref + "']");
    is(cb.length, 1, "found checkbox for " + pref + " preference");
    if (cb[0].checked != aCheckState)
      cb[0].click();
  },

  


  resetCheckboxes: function () {
    var cb = this.win.document.querySelectorAll("#itemList > [preference]");
    ok(cb.length > 1, "found checkboxes for preferences");
    for (var i = 0; i < cb.length; ++i) {
      var pref = this.win.document.getElementById(cb[i].getAttribute("preference"));
      if (pref.value != pref.defaultValue)
        cb[i].click();
    }
  },

  


  getDetailsButton: function () {
    return this.win.document.getElementById("detailsExpander");
  },

  


  getDurationDropdown: function () {
    return this.win.document.getElementById("sanitizeDurationChoice");
  },

  


  getItemList: function () {
    return this.win.document.getElementById("itemList");
  },

  


  getWarningPanel: function () {
    return this.win.document.getElementById("sanitizeEverythingWarningBox");
  },

  



  isWarningPanelVisible: function () {
    return !this.getWarningPanel().hidden;
  },

  





  open: function () {
    let wh = this;

    let windowObserver = {
      observe: function(aSubject, aTopic, aData) {
        if (aTopic !== "domwindowopened")
          return;

        winWatch.unregisterNotification(this);

        var loaded = false;
        let win = aSubject.QueryInterface(Ci.nsIDOMWindow);

        win.addEventListener("load", function onload(event) {
          win.removeEventListener("load", onload, false);

          if (win.name !== "SanitizeDialog")
            return;

          wh.win = win;
          loaded = true;

          executeSoon(function () {
            
            
            try {
              wh.onload();
            }
            catch (exc) {
              win.close();
              ok(false, "Unexpected exception: " + exc + "\n" + exc.stack);
              finish();
            }
          });
        }, false);

        win.addEventListener("unload", function onunload(event) {
          if (win.name !== "SanitizeDialog") {
            win.removeEventListener("unload", onunload, false);
            return;
          }

          
          if (!loaded)
            return;

          win.removeEventListener("unload", onunload, false);
          wh.win = win;

          executeSoon(function () {
            
            
            try {
              if (wh.onunload)
                wh.onunload();
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
    };
    winWatch.registerNotification(windowObserver);
    winWatch.openWindow(null,
                        "chrome://browser/content/sanitize.xul",
                        "SanitizeDialog",
                        "chrome,titlebar,dialog,centerscreen,modal",
                        null);
  },

  





  selectDuration: function (aDurVal) {
    this.getDurationDropdown().value = aDurVal;
    if (aDurVal === Sanitizer.TIMESPAN_EVERYTHING) {
      is(this.isWarningPanelVisible(), true,
         "Warning panel should be visible for TIMESPAN_EVERYTHING");
    }
    else {
      is(this.isWarningPanelVisible(), false,
         "Warning panel should not be visible for non-TIMESPAN_EVERYTHING");
    }
  },

  


  toggleDetails: function () {
    this.getDetailsButton().click();
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











function boolPrefIs(aPrefName, aExpectedVal, aMsg) {
  is(gPrefService.getBoolPref("privacy." + aPrefName), aExpectedVal, aMsg);
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
  let rows = stmt.step();
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











function intPrefIs(aPrefName, aExpectedVal, aMsg) {
  is(gPrefService.getIntPref("privacy." + aPrefName), aExpectedVal, aMsg);
}



function test() {
  blankSlate();
  waitForExplicitFinish();
  
  doNextTest();
}
