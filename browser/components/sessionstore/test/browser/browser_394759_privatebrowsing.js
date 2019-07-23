






































function test() {
  

  waitForExplicitFinish();

  
  let pb = Cc["@mozilla.org/privatebrowsing;1"].
           getService(Ci.nsIPrivateBrowsingService);
  let ss = Cc["@mozilla.org/browser/sessionstore;1"].
           getService(Ci.nsISessionStore);
  let ww = Cc["@mozilla.org/embedcomp/window-watcher;1"].
           getService(Ci.nsIWindowWatcher);

  
  let profilePath = Cc["@mozilla.org/file/directory_service;1"].
                    getService(Ci.nsIProperties).
                    get("ProfD", Ci.nsIFile);
  let sessionStoreJS = profilePath.clone();
  sessionStoreJS.append("sessionstore.js");
  if (sessionStoreJS.exists())
    sessionStoreJS.remove(false);
  ok(sessionStoreJS.exists() == false, "sessionstore.js was removed");
  
  
  gPrefService.setIntPref("browser.sessionstore.interval", 0);
  
  sessionStoreJS = profilePath.clone();
  sessionStoreJS.append("sessionstore.js");

  
  
  let blankState = JSON.stringify({
    windows: [{
      tabs: [{ entries: [{ url: "about:blank" }] }],
      _closedTabs: []
    }],
    _closedWindows: []
  });
  ss.setBrowserState(blankState);

  let closedWindowCount = ss.getClosedWindowCount();
  is(closedWindowCount, 0, "Correctly set window count");

  
  const NOW = Date.now();

  const TESTS = [
    { url: "about:config",
      key: "bug 394759 Non-PB",
      value: "uniq" + (++NOW) },
    { url: "about:mozilla",
      key: "bug 394759 PB",
      value: "uniq" + (++NOW) },
  ];

  function openWindowAndTest(aTestIndex, aRunNextTestInPBMode) {
    info("Opening new window");
    let windowObserver = {
      observe: function(aSubject, aTopic, aData) {
        if (aTopic === "domwindowopened") {
          ww.unregisterNotification(this);
          info("New window has been opened");
          let win = aSubject.QueryInterface(Ci.nsIDOMWindow);
          win.addEventListener("load", function onLoad(event) {
            win.removeEventListener("load", onLoad, false);
            info("New window has been loaded");
            win.gBrowser.addEventListener("load", function(aEvent) {
              win.gBrowser.removeEventListener("load", arguments.callee, true);
              info("New window browser has been loaded");
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
                    ok(data.toSource().indexOf(TESTS[aTestIndex].value) > -1,
                       "The data associated with the recently closed window was " +
                       "restored when exiting PB mode");
                  }

                  if (aTestIndex == TESTS.length - 1) {
                    
                    gPrefService.clearUserPref("browser.sessionstore.interval");
                    finish();
                  }
                  else {
                    
                    openWindowAndTest(aTestIndex + 1, !aRunNextTestInPBMode);
                  }
                });
              });
            }, true);
          }, false);
        }
      }
    };
    ww.registerNotification(windowObserver);
    
    openDialog(location, "_blank", "chrome,all,dialog=no", TESTS[aTestIndex].url);
  }

  openWindowAndTest(0, true);
}
