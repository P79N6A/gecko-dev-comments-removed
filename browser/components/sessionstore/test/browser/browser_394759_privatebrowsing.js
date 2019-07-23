






































function test() {
  

  waitForExplicitFinish();

  let ss = Cc["@mozilla.org/browser/sessionstore;1"].
           getService(Ci.nsISessionStore);

  
  let profilePath = Cc["@mozilla.org/file/directory_service;1"].
                    getService(Ci.nsIProperties).
                    get("ProfD", Ci.nsIFile);
  let sessionStoreJS = profilePath.clone();
  sessionStoreJS.append("sessionstore.js");
  if (sessionStoreJS.exists())
    sessionStoreJS.remove(false);
  ok(sessionStoreJS.exists() == false, "sessionstore.js was removed");
  
  
  gPrefService.setIntPref("browser.sessionstore.interval", 100);
  
  sessionStoreJS = profilePath.clone();
  sessionStoreJS.append("sessionstore.js");

  
  let os = Cc["@mozilla.org/observer-service;1"].
           getService(Ci.nsIObserverService);
  os.addObserver({observe: function(aSubject, aTopic, aData) {
    if (gPrefService.prefHasUserValue("browser.sessionstore.interval"))
      gPrefService.clearUserPref("browser.sessionstore.interval");
    os.removeObserver(this, aTopic);
    executeSoon(continue_test);
  }}, "sessionstore-state-write-complete", false);

  
  os.addObserver({observe: function(aSubject, aTopic, aData) {
    
    
    
    info("Windows status has been restored, was that expected?");
    os.removeObserver(this, aTopic);
  }}, "sessionstore-windows-restored", false);

  
  
  let blankState = JSON.stringify({
    windows: [{
      tabs: [{ entries: [{ url: "about:blank" }] }],
      _closedTabs: []
    }],
    _closedWindows: []
  });
  ss.setBrowserState(blankState);
}

function continue_test() {
  let ww = Cc["@mozilla.org/embedcomp/window-watcher;1"].
           getService(Ci.nsIWindowWatcher);
  let pb = Cc["@mozilla.org/privatebrowsing;1"].
           getService(Ci.nsIPrivateBrowsingService);
  let ss = Cc["@mozilla.org/browser/sessionstore;1"].
           getService(Ci.nsISessionStore);

  let closedWindowCount = ss.getClosedWindowCount();
  is(closedWindowCount, 0, "Correctly set window count");

  
  let now = Date.now();
  const TESTS = [
    { url: "about:config",
      key: "bug 394759 Non-PB",
      value: "uniq" + (++now) },
    { url: "about:mozilla",
      key: "bug 394759 PB",
      value: "uniq" + (++now) },
  ];

  let loadWasCalled = false;
  function openWindowAndTest(aTestIndex, aRunNextTestInPBMode) {
    info("Opening new window");
    let windowObserver = {
      observe: function(aSubject, aTopic, aData) {
        if (aTopic === "domwindowopened") {
          info("New window has been opened");
          let win = aSubject.QueryInterface(Ci.nsIDOMWindow);
          win.addEventListener("load", function onLoad(event) {
            win.removeEventListener("load", onLoad, false);
            info("New window has been loaded");
            win.gBrowser.addEventListener("load", function(aEvent) {
              win.gBrowser.removeEventListener("load", arguments.callee, true);
              info("New window browser has been loaded");
              loadWasCalled = true;
              executeSoon(function() {
                
                win.gBrowser.addTab();

                executeSoon(function() {
                  
                  ss.setWindowValue(win, TESTS[aTestIndex].key, TESTS[aTestIndex].value);

                  win.close();

                  
                  is(ss.getClosedWindowCount(), closedWindowCount + 1,
                     "The closed window was added to the list");

                  
                  let data = JSON.parse(ss.getClosedWindowData())[0];
                  ok(data.toSource().indexOf(TESTS[aTestIndex].value) > -1,
                     "The closed window data was stored correctly");

                  if (aRunNextTestInPBMode) {
                    
                    pb.privateBrowsingEnabled = true;
                    ok(pb.privateBrowsingEnabled, "private browsing enabled");

                    
                    is(ss.getClosedWindowCount(), 0,
                       "Recently Closed Windows are removed when entering Private Browsing");
                    is(ss.getClosedWindowData(), "[]",
                       "Recently Closed Windows data is cleared when entering Private Browsing");
                  }
                  else {
                    
                    pb.privateBrowsingEnabled = false;
                    ok(!pb.privateBrowsingEnabled, "private browsing disabled");

                    
                    is(ss.getClosedWindowCount(), closedWindowCount + 1,
                       "The correct number of recently closed windows were restored " +
                       "when exiting PB mode");

                    let data = JSON.parse(ss.getClosedWindowData())[0];
                    ok(data.toSource().indexOf(TESTS[aTestIndex - 1].value) > -1,
                       "The data associated with the recently closed window was " +
                       "restored when exiting PB mode");
                  }

                  if (aTestIndex == TESTS.length - 1)
                    finish();
                  else {
                    
                    openWindowAndTest(aTestIndex + 1, !aRunNextTestInPBMode);
                  }
                });
              });
            }, true);
          }, false);
        }
        else if (aTopic === "domwindowclosed") {
          info("Window closed");
          ww.unregisterNotification(this);
          if (!loadWasCalled) {
            ok(false, "Window was closed before load could fire!");
            finish();
          }
        }
      }
    };
    ww.registerNotification(windowObserver);
    
    openDialog(location, "_blank", "chrome,all,dialog=no", TESTS[aTestIndex].url);
  }

  openWindowAndTest(0, true);
}
