




































let ss = Cc["@mozilla.org/browser/sessionstore;1"].
         getService(Ci.nsISessionStore);

let stateBackup = ss.getBrowserState();

function test() {
  
  waitForExplicitFinish();
  testBug600545();
}

function testBug600545() {
  
  Services.prefs.setBoolPref("browser.sessionstore.resume_from_crash", false);

  
  
  function waitForSaveState(aSaveStateCallback) {
    let topic = "sessionstore-state-write";
    Services.obs.addObserver(function() {
      Services.obs.removeObserver(arguments.callee, topic, false);
      executeSoon(aSaveStateCallback);
    }, topic, false);
  };

  
  function waitForBrowserState(aState, aSetStateCallback) {
    let locationChanges = 0;
    let tabsRestored = getStateTabCount(aState);

    
    let progressListener = {
      onLocationChange: function (aBrowser) {
        if (++locationChanges == tabsRestored) {
          
          
          window.gBrowser.removeTabsProgressListener(this);
          executeSoon(aSetStateCallback);
        }
      }
    }

    
    function windowObserver(aSubject, aTopic, aData) {
      let theWin = aSubject.QueryInterface(Ci.nsIDOMWindow);
      if (aTopic == "domwindowopened") {
        theWin.addEventListener("load", function() {
          theWin.removeEventListener("load", arguments.callee, false);

          Services.ww.unregisterNotification(windowObserver);
          theWin.gBrowser.addTabsProgressListener(progressListener);
        }, false);
      }
    }

    Services.ww.registerNotification(windowObserver);
    window.gBrowser.addTabsProgressListener(progressListener);
    ss.setBrowserState(JSON.stringify(aState));
  }

  
  
  
  
  
  let state = { windows: [
    {
      tabs: [
        { entries: [{ url: "http://example.org#0" }], pinned:true },
        { entries: [{ url: "http://example.com#1" }] },
        { entries: [{ url: "http://example.com#2" }] },
      ],
      selected: 2
    },
    {
      tabs: [
        { entries: [{ url: "http://example.com#3" }] },
        { entries: [{ url: "http://example.com#4" }] },
        { entries: [{ url: "http://example.com#5" }] },
        { entries: [{ url: "http://example.com#6" }] }
      ],
      selected: 3
    }
  ] };

  waitForBrowserState(state, function() {
    waitForSaveState(function () {
      let expectedNumberOfTabs = getStateTabCount(state);
      let retrievedState = JSON.parse(ss.getBrowserState());
      let actualNumberOfTabs = getStateTabCount(retrievedState);

      is(actualNumberOfTabs, expectedNumberOfTabs,
        "Number of tabs in retreived session data, matches number of tabs set.");

      done();
    });
  });
}

function done() {
  
  try {
    Services.prefs.clearUserPref("browser.sessionstore.resume_from_crash");
  } catch (e) {}

  ss.setBrowserState(stateBackup);
  executeSoon(finish);
}


function getStateTabCount(aState) {
  let tabCount = 0;
  for (let i in aState.windows)
    tabCount += aState.windows[i].tabs.length;
  return tabCount;
}