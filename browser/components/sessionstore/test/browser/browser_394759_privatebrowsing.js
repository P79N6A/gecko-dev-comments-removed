






































function browserWindowsCount() {
  let count = 0;
  let e = Cc["@mozilla.org/appshell/window-mediator;1"]
            .getService(Ci.nsIWindowMediator)
            .getEnumerator("navigator:browser");
  while (e.hasMoreElements()) {
    if (!e.getNext().closed)
      ++count;
  }
  return count;
}

function test() {
  
  is(browserWindowsCount(), 1, "Only one browser window should be open initially");

  waitForExplicitFinish();

  
  gPrefService.setIntPref("browser.sessionstore.interval", 100000);

  
  
  let ss = Cc["@mozilla.org/browser/sessionstore;1"].
           getService(Ci.nsISessionStore);
  let blankState = JSON.stringify({
    windows: [{
      tabs: [{ entries: [{ url: "about:blank" }] }],
      _closedTabs: []
    }],
    _closedWindows: []
  });
  ss.setBrowserState(blankState);

  
  
  
  let os = Cc["@mozilla.org/observer-service;1"].
           getService(Ci.nsIObserverService);
  os.addObserver(function (aSubject, aTopic, aData) {
    os.removeObserver(arguments.callee, aTopic);
    info("sessionstore.js is being written");
    executeSoon(continue_test);
  }, "sessionstore-state-write", false);

  
  let profilePath = Cc["@mozilla.org/file/directory_service;1"].
                    getService(Ci.nsIProperties).
                    get("ProfD", Ci.nsIFile);
  let sessionStoreJS = profilePath.clone();
  sessionStoreJS.append("sessionstore.js");
  if (sessionStoreJS.exists())
    sessionStoreJS.remove(false);
  info("sessionstore.js was correctly removed: " + (!sessionStoreJS.exists()));

  
  
  gPrefService.setIntPref("browser.sessionstore.interval", 0);
}

function continue_test() {
  let pb = Cc["@mozilla.org/privatebrowsing;1"].
           getService(Ci.nsIPrivateBrowsingService);
  
  ok(!pb.privateBrowsingEnabled, "Private Browsing is disabled");
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

  function openWindowAndTest(aTestIndex, aRunNextTestInPBMode) {
    info("Opening new window");
    function onLoad(event) {
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
                    ok(data.toSource().indexOf(TESTS[aTestIndex - 1].value) > -1,
                       "The data associated with the recently closed window was " +
                       "restored when exiting PB mode");
                  }

                  if (aTestIndex == TESTS.length - 1) {
                    if (gPrefService.prefHasUserValue("browser.sessionstore.interval"))
                      gPrefService.clearUserPref("browser.sessionstore.interval");
                    is(browserWindowsCount(), 1, "Only one browser window should be open eventually");
                    finish();
                  }
                  else {
                    
                    openWindowAndTest(aTestIndex + 1, !aRunNextTestInPBMode);
                  }
                });
              });
            }, true);
    }
    
    var win = openDialog(location, "", "chrome,all,dialog=no", TESTS[aTestIndex].url);
    win.addEventListener("load", onLoad, false);
  }

  openWindowAndTest(0, true);
}
