


















Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
let {LoadContextInfo} = Cu.import("resource://gre/modules/LoadContextInfo.jsm", {});

XPCOMUtils.defineLazyModuleGetter(this, "FormHistory",
                                  "resource://gre/modules/FormHistory.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Downloads",
                                  "resource://gre/modules/Downloads.jsm");

let tempScope = {};
Cc["@mozilla.org/moz/jssubscript-loader;1"].getService(Ci.mozIJSSubScriptLoader)
                                           .loadSubScript("chrome://browser/content/sanitize.js", tempScope);
let Sanitizer = tempScope.Sanitizer;

const kMsecPerMin = 60 * 1000;
const kUsecPerMin = 60 * 1000000;

let formEntries, downloadIDs, olderDownloadIDs;


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
    let places = [];
    let pURI;
    for (let i = 0; i < 30; i++) {
      pURI = makeURI("http://" + i + "-minutes-ago.com/");
      places.push({uri: pURI, visitDate: visitTimeForMinutesAgo(i)});
      uris.push(pURI);
    }

    PlacesTestUtils.addVisits(places).then(() => {
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
      };
      wh.onunload = function () {
        yield promiseHistoryClearedState(uris, false);
        yield blankSlate();
        yield promiseHistoryClearedState(uris, true);
      };
      wh.open();
    });
  },

  function () {
    
    Task.spawn(function () {
      downloadIDs = [];
      for (let i = 0; i < 5; i++) {
        yield addDownloadWithMinutesAgo(downloadIDs, i);
      }
      
      olderDownloadIDs = [];
      for (let i = 0; i < 5; i++) {
        yield addDownloadWithMinutesAgo(olderDownloadIDs, 61 + i);
      }

      doNextTest();
    }).then(null, Components.utils.reportError);
  },

  



  function () {
    
    let uris = [];
    let places = [];
    let pURI;
    for (let i = 0; i < 30; i++) {
      pURI = makeURI("http://" + i + "-minutes-ago.com/");
      places.push({uri: pURI, visitDate: visitTimeForMinutesAgo(i)});
      uris.push(pURI);
    }
    
    let olderURIs = [];
    for (let i = 0; i < 5; i++) {
      pURI = makeURI("http://" + (61 + i) + "-minutes-ago.com/");
      places.push({uri: pURI, visitDate: visitTimeForMinutesAgo(61 + i)});
      olderURIs.push(pURI);
    }

    PlacesTestUtils.addVisits(places).then(() => {
      let totalHistoryVisits = uris.length + olderURIs.length;

      let wh = new WindowHelper();
      wh.onload = function () {
        this.selectDuration(Sanitizer.TIMESPAN_HOUR);
        this.checkPrefCheckbox("history", true);
        this.acceptDialog();
      };
      wh.onunload = function () {
        intPrefIs("sanitize.timeSpan", Sanitizer.TIMESPAN_HOUR,
                  "timeSpan pref should be hour after accepting dialog with " +
                  "hour selected");
        boolPrefIs("cpd.history", true,
                   "history pref should be true after accepting dialog with " +
                   "history checkbox checked");
        boolPrefIs("cpd.downloads", true,
                   "downloads pref should be true after accepting dialog with " +
                   "history checkbox checked");

        
        yield promiseHistoryClearedState(uris, true);
        yield ensureDownloadsClearedState(downloadIDs, true);

        
        yield promiseHistoryClearedState(olderURIs, false);
        yield ensureDownloadsClearedState(olderDownloadIDs, false);

        
        yield blankSlate();
        yield promiseHistoryClearedState(olderURIs, true);
        yield ensureDownloadsClearedState(olderDownloadIDs, true);
      };
      wh.open();
    });
  },

  


  function () {
    formEntries = [];

    let iter = function() {
      for (let i = 0; i < 5; i++) {
        formEntries.push(addFormEntryWithMinutesAgo(iter, i));
        yield undefined;
      }
      doNextTest();
    }();

    iter.next();
  },

  function () {
    
    Task.spawn(function () {
      downloadIDs = [];
      for (let i = 0; i < 5; i++) {
        yield addDownloadWithMinutesAgo(downloadIDs, i);
      }

      doNextTest();
    }).then(null, Components.utils.reportError);
  },

  



  function () {
    
    let uris = [];
    let places = [];
    let pURI;
    for (let i = 0; i < 5; i++) {
      pURI = makeURI("http://" + i + "-minutes-ago.com/");
      places.push({uri: pURI, visitDate: visitTimeForMinutesAgo(i)});
      uris.push(pURI);
    }

    PlacesTestUtils.addVisits(places).then(() => {
      let wh = new WindowHelper();
      wh.onload = function () {
        is(this.isWarningPanelVisible(), false,
           "Warning panel should be hidden after previously accepting dialog " +
           "with a predefined timespan");
        this.selectDuration(Sanitizer.TIMESPAN_HOUR);

        
        this.checkPrefCheckbox("history", false);
        this.checkPrefCheckbox("formdata", true);
        this.acceptDialog();
      };
      wh.onunload = function () {
        intPrefIs("sanitize.timeSpan", Sanitizer.TIMESPAN_HOUR,
                  "timeSpan pref should be hour after accepting dialog with " +
                  "hour selected");
        boolPrefIs("cpd.history", false,
                   "history pref should be false after accepting dialog with " +
                   "history checkbox unchecked");
        boolPrefIs("cpd.downloads", false,
                   "downloads pref should be false after accepting dialog with " +
                   "history checkbox unchecked");

        
        yield promiseHistoryClearedState(uris, false);
        yield ensureDownloadsClearedState(downloadIDs, false);

        formEntries.forEach(function (entry) {
          let exists = yield formNameExists(entry);
          is(exists, false, "form entry " + entry + " should no longer exist");
        });

        
        yield blankSlate();
        yield promiseHistoryClearedState(uris, true);
        yield ensureDownloadsClearedState(downloadIDs, true);
      };
      wh.open();
    });
  },

  


  function () {
    
    let uris = [];
    let places = [];
    let pURI;
    
    
    [10, 70, 130, 250].forEach(function(aValue) {
      pURI = makeURI("http://" + aValue + "-minutes-ago.com/");
      places.push({uri: pURI, visitDate: visitTimeForMinutesAgo(aValue)});
      uris.push(pURI);
    });
    PlacesTestUtils.addVisits(places).then(() => {
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
      };
      wh.onunload = function () {
        intPrefIs("sanitize.timeSpan", Sanitizer.TIMESPAN_EVERYTHING,
                  "timeSpan pref should be everything after accepting dialog " +
                  "with everything selected");

        yield promiseHistoryClearedState(uris, true);
      };
      wh.open();
    });
  },

  



  function () {
    
    let uris = [];
    let places = [];
    let pURI;
    
    
    [10, 70, 130, 250].forEach(function(aValue) {
      pURI = makeURI("http://" + aValue + "-minutes-ago.com/");
      places.push({uri: pURI, visitDate: visitTimeForMinutesAgo(aValue)});
      uris.push(pURI);
    });
    PlacesTestUtils.addVisits(places).then(() => {
      let wh = new WindowHelper();
      wh.onload = function () {
        is(this.isWarningPanelVisible(), true,
           "Warning panel should be visible after previously accepting dialog " +
           "with clearing everything");
        this.selectDuration(Sanitizer.TIMESPAN_EVERYTHING);
        this.checkPrefCheckbox("history", true);
        this.acceptDialog();
      };
      wh.onunload = function () {
        intPrefIs("sanitize.timeSpan", Sanitizer.TIMESPAN_EVERYTHING,
                  "timeSpan pref should be everything after accepting dialog " +
                  "with everything selected");

        yield promiseHistoryClearedState(uris, true);
      };
      wh.open();
    });
  },

  


  function () {
    let iter = function() {
      formEntries = [ addFormEntryWithMinutesAgo(iter, 10) ];
      yield undefined;
      doNextTest();
    }();

    iter.next();
  },

  




  function () {
    
    let pURI = makeURI("http://" + 10 + "-minutes-ago.com/");
    PlacesTestUtils.addVisits({uri: pURI, visitDate: visitTimeForMinutesAgo(10)}).then(() => {
      let uris = [ pURI ];

      let wh = new WindowHelper();
      wh.onload = function() {
        
        var cb = this.win.document.querySelectorAll(
                   "#itemList > [preference='privacy.cpd.formdata']");
        ok(cb.length == 1 && !cb[0].disabled, "There is formdata, checkbox to " +
           "clear formdata should be enabled.");

        var cb = this.win.document.querySelectorAll(
                   "#itemList > [preference='privacy.cpd.history']");
        ok(cb.length == 1 && !cb[0].disabled, "There is history, checkbox to " +
           "clear history should be enabled.");

        this.checkAllCheckboxes();
        this.acceptDialog();
      };
      wh.onunload = function () {
        yield promiseHistoryClearedState(uris, true);

        let exists = yield formNameExists(formEntries[0]);
        is(exists, false, "form entry " + formEntries[0] + " should no longer exist");
      };
      wh.open();
    });
  },
  function () {
    let wh = new WindowHelper();
    wh.onload = function() {
      boolPrefIs("cpd.history", true,
                 "history pref should be true after accepting dialog with " +
                 "history checkbox checked");
      boolPrefIs("cpd.formdata", true,
                 "formdata pref should be true after accepting dialog with " +
                 "formdata checkbox checked");


      
      
      var cb = this.win.document.querySelectorAll(
                 "#itemList > [preference='privacy.cpd.formdata']");
      ok(cb.length == 1 && cb[0].disabled && !cb[0].checked,
         "There is no formdata history, checkbox should be disabled and be " +
         "cleared to reduce user confusion (bug 497664).");

      var cb = this.win.document.querySelectorAll(
                 "#itemList > [preference='privacy.cpd.history']");
      ok(cb.length == 1 && !cb[0].disabled && cb[0].checked,
         "There is no history, but history checkbox should always be enabled " +
         "and will be checked from previous preference.");

      this.acceptDialog();
    }
    wh.open();
  },

  


  function () {
    let iter = function() {
      formEntries = [ addFormEntryWithMinutesAgo(iter, 10) ];
      yield undefined;
      doNextTest();
    }();

    iter.next();
  },

  function () {
    let wh = new WindowHelper();
    wh.onload = function() {
      boolPrefIs("cpd.formdata", true,
                 "formdata pref should persist previous value after accepting " +
                 "dialog where you could not clear formdata.");

      var cb = this.win.document.querySelectorAll(
                 "#itemList > [preference='privacy.cpd.formdata']");
      ok(cb.length == 1 && !cb[0].disabled && cb[0].checked,
         "There exists formEntries so the checkbox should be in sync with " +
         "the pref.");

      this.acceptDialog();
    };
    wh.onunload = function () {
      let exists = yield formNameExists(formEntries[0]);
      is(exists, false, "form entry " + formEntries[0] + " should no longer exist");
    };
    wh.open();
  },


  



  function () {
    let wh = new WindowHelper();
    wh.onload = function () {
      
      this.checkAllCheckboxes();
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

      
      this.checkAllCheckboxes();
      this.checkPrefCheckbox("siteSettings", false);
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
  },
  function () {
    

    
    var URL = "http://www.example.com";

    var ios = Cc["@mozilla.org/network/io-service;1"]
              .getService(Ci.nsIIOService);
    var URI = ios.newURI(URL, null, null);

    var sm = Cc["@mozilla.org/scriptsecuritymanager;1"]
             .getService(Ci.nsIScriptSecurityManager);
    var principal = sm.getNoAppCodebasePrincipal(URI);

    
    var pm = Cc["@mozilla.org/permissionmanager;1"]
             .getService(Ci.nsIPermissionManager);
    pm.addFromPrincipal(principal, "offline-app", Ci.nsIPermissionManager.ALLOW_ACTION);
    pm.addFromPrincipal(principal, "offline-app", Ci.nsIOfflineCacheUpdateService.ALLOW_NO_WARN);

    
    var appcacheserv = Cc["@mozilla.org/network/application-cache-service;1"]
                       .getService(Ci.nsIApplicationCacheService);
    var appcachegroupid = appcacheserv.buildGroupID(makeURI(URL + "/manifest"), LoadContextInfo.default);
    var appcache = appcacheserv.createApplicationCache(appcachegroupid);

    var cacheserv = Cc["@mozilla.org/netwerk/cache-storage-service;1"]
                    .getService(Ci.nsICacheStorageService);
    var storage = cacheserv.appCacheStorage(LoadContextInfo.default, appcache);

    
    let wh = new WindowHelper();
    wh.onload = function () {
      this.selectDuration(Sanitizer.TIMESPAN_EVERYTHING);
      
      this.toggleDetails();
      
      this.uncheckAllCheckboxes();
      this.checkPrefCheckbox("offlineApps", true);
      this.acceptDialog();
    };
    wh.onunload = function () {
      
      var size = -1;
      var visitor = {
        onCacheStorageInfo: function (aEntryCount, aConsumption, aCapacity, aDiskDirectory)
        {
          size = aConsumption;
        }
      };
      storage.asyncVisitStorage(visitor, false);
      
      is(size, 0, "offline application cache entries evicted");
    };

    var cacheListener = {
      onCacheEntryCheck: function() { return Ci.nsICacheEntryOpenCallback.ENTRY_WANTED; },
      onCacheEntryAvailable: function (entry, isnew, appcache, status) {
        is(status, Cr.NS_OK);
        var stream = entry.openOutputStream(0);
        var content = "content";
        stream.write(content, content.length);
        stream.close();
        entry.close();
        wh.open();
      }
    };

    storage.asyncOpenURI(makeURI(URL), "", Ci.nsICacheStorage.OPEN_TRUNCATE, cacheListener);
  },
  function () {
    

    
    var URL = "http://www.example.com";

    var ios = Cc["@mozilla.org/network/io-service;1"]
              .getService(Ci.nsIIOService);
    var URI = ios.newURI(URL, null, null);

    var sm = Cc["@mozilla.org/scriptsecuritymanager;1"]
             .getService(Ci.nsIScriptSecurityManager);
    var principal = sm.getNoAppCodebasePrincipal(URI);

    
    let wh = new WindowHelper();
    wh.onload = function () {
      this.selectDuration(Sanitizer.TIMESPAN_EVERYTHING);
      
      this.toggleDetails();
      
      this.uncheckAllCheckboxes();
      this.checkPrefCheckbox("siteSettings", true);
      this.acceptDialog();
    };
    wh.onunload = function () {
      
      var pm = Cc["@mozilla.org/permissionmanager;1"]
               .getService(Ci.nsIPermissionManager);
      is(pm.testPermissionFromPrincipal(principal, "offline-app"), 0, "offline-app permissions removed");
    };
    wh.open();
  }
];



var gCurrTest = 0;

let now_mSec = Date.now();
let now_uSec = now_mSec * 1000;










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
    if (!hidden) {
      ok(list.boxObject.height > 30, "listbox has sufficient size")
      height += list.boxObject.height;
    }
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

  


  _checkAllCheckboxesCustom: function (check) {
    var cb = this.win.document.querySelectorAll("#itemList > [preference]");
    ok(cb.length > 1, "found checkboxes for preferences");
    for (var i = 0; i < cb.length; ++i) {
      var pref = this.win.document.getElementById(cb[i].getAttribute("preference"));
      if (!!pref.value ^ check)
        cb[i].click();
    }
  },

  checkAllCheckboxes: function () {
    this._checkAllCheckboxesCustom(true);
  },

  uncheckAllCheckboxes: function () {
    this._checkAllCheckboxesCustom(false);
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

    function windowObserver(aSubject, aTopic, aData) {
      if (aTopic != "domwindowopened")
        return;

      Services.ww.unregisterNotification(windowObserver);

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
            if (wh.onunload) {
              Task.spawn(wh.onunload).then(function() {
                waitForAsyncUpdates(doNextTest);
              }).then(null, Components.utils.reportError);
            } else {
              waitForAsyncUpdates(doNextTest);
            }
          }
          catch (exc) {
            win.close();
            ok(false, "Unexpected exception: " + exc + "\n" + exc.stack);
            finish();
          }
        });
      }, false);
    }
    Services.ww.registerNotification(windowObserver);
    Services.ww.openWindow(null,
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







function addDownloadWithMinutesAgo(aExpectedPathList, aMinutesAgo) {
  let publicList = yield Downloads.getList(Downloads.PUBLIC);

  let name = "fakefile-" + aMinutesAgo + "-minutes-ago";
  let download = yield Downloads.createDownload({
    source: "https://bugzilla.mozilla.org/show_bug.cgi?id=480169",
    target: name
  });
  download.startTime = new Date(now_mSec - (aMinutesAgo * kMsecPerMin));
  download.canceled = true;
  publicList.add(download);

  ok((yield downloadExists(name)),
     "Sanity check: download " + name +
     " should exist after creating it");

  aExpectedPathList.push(name);
}







function addFormEntryWithMinutesAgo(then, aMinutesAgo) {
  let name = aMinutesAgo + "-minutes-ago";

  
  let timestamp = now_uSec - (aMinutesAgo * kUsecPerMin);

  FormHistory.update({ op: "add", fieldname: name, value: "dummy", firstUsed: timestamp },
                     { handleError: function (error) {
                         do_throw("Error occurred updating form history: " + error);
                       },
                       handleCompletion: function (reason) { then.next(); }
                     });
  return name;
}




function formNameExists(name)
{
  let deferred = Promise.defer();

  let count = 0;
  FormHistory.count({ fieldname: name },
                    { handleResult: function (result) count = result,
                      handleError: function (error) {
                        do_throw("Error occurred searching form history: " + error);
                        deferred.reject(error);
                      },
                      handleCompletion: function (reason) {
                          if (!reason) deferred.resolve(count);
                      }
                    });

  return deferred.promise;
}




function blankSlate() {
  let deleteDownloads = Task.spawn(function* deleteAllDownloads() {
    let publicList = yield Downloads.getList(Downloads.PUBLIC);
    let downloads = yield publicList.getAll();
    for (let download of downloads) {
      yield publicList.remove(download);
      yield download.finalize(true);
    }
  }).then(null, Components.utils.reportError);

  let updateFormHistory = new Promise((resolve, reject) => {
    FormHistory.update({op: "remove"}, {
      handleCompletion(reason) {
        if (!reason) {
          resolve();
        }
      },

      handleError(error) {
        do_throw("Error occurred updating form history: " + error);
        reject(error);
      }
    });
  });

  return Promise.all([
    PlacesTestUtils.clearHistory(), deleteDownloads, updateFormHistory]);
}











function boolPrefIs(aPrefName, aExpectedVal, aMsg) {
  is(gPrefService.getBoolPref("privacy." + aPrefName), aExpectedVal, aMsg);
}








function downloadExists(aPath)
{
  return Task.spawn(function() {
    let publicList = yield Downloads.getList(Downloads.PUBLIC);
    let listArray = yield publicList.getAll();
    throw new Task.Result(listArray.some(i => i.target.path == aPath));
  });
}





function doNextTest() {
  if (gAllTests.length <= gCurrTest) {
    blankSlate();
    waitForAsyncUpdates(finish);
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
    is((yield downloadExists(id)), !aShouldBeCleared,
       "download " + id + " should " + niceStr + " exist");
  });
}











function intPrefIs(aPrefName, aExpectedVal, aMsg) {
  is(gPrefService.getIntPref("privacy." + aPrefName), aExpectedVal, aMsg);
}







function visitTimeForMinutesAgo(aMinutesAgo) {
  return now_uSec - aMinutesAgo * kUsecPerMin;
}



function test() {
  requestLongerTimeout(2);
  waitForExplicitFinish();
  blankSlate();
  
  waitForAsyncUpdates(doNextTest);
}
