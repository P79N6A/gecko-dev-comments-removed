




function test() {
  waitForExplicitFinish();

  let windowsToClose = [];
  let closedWindowCount = 0;
  
  let now = Date.now();
  const TESTS = [
    { url: "about:config",
      key: "bug 394759 Non-PB",
      value: "uniq" + (++now) },
    { url: "about:mozilla",
      key: "bug 394759 PB",
      value: "uniq" + (++now) },
  ];

  registerCleanupFunction(function() {
    Services.prefs.clearUserPref("browser.sessionstore.interval");
    windowsToClose.forEach(function(win) {
      win.close();
    });
  });

  function testOpenCloseWindow(aIsPrivate, aTest, aCallback) {
    whenNewWindowLoaded(aIsPrivate, function(win) {
      win.gBrowser.selectedBrowser.addEventListener("load", function onLoad() {
        win.gBrowser.selectedBrowser.removeEventListener("load", onLoad, true);
        executeSoon(function() {
          
          ss.setWindowValue(win, aTest.key, aTest.value);
          
          win.close();
          aCallback();
        });
      }, true);
      win.gBrowser.selectedBrowser.loadURI(aTest.url);
    });
  }

  function testOnWindow(aIsPrivate, aValue, aCallback) {
    whenNewWindowLoaded(aIsPrivate, function(win) {
      windowsToClose.push(win);
      executeSoon(function() checkClosedWindows(aIsPrivate, aValue, aCallback));
    });
  }

  function checkClosedWindows(aIsPrivate, aValue, aCallback) {
    let data = JSON.parse(ss.getClosedWindowData())[0];
    is(ss.getClosedWindowCount(), 1, "Check the closed window count");
    ok(JSON.stringify(data).indexOf(aValue) > -1,
       "Check the closed window data was stored correctly");
    aCallback();
  }

  function setupBlankState(aCallback) {
    
    
    Services.prefs.setIntPref("browser.sessionstore.interval", 100000);

    
    
    let blankState = JSON.stringify({
      windows: [{
        tabs: [{ entries: [{ url: "about:blank" }] }],
        _closedTabs: []
      }],
      _closedWindows: []
    });
    ss.setBrowserState(blankState);

    
    
    
    Services.obs.addObserver(function (aSubject, aTopic, aData) {
      Services.obs.removeObserver(arguments.callee, aTopic);
      info("sessionstore.js is being written");

      closedWindowCount = ss.getClosedWindowCount();
      is(closedWindowCount, 0, "Correctly set window count");

      executeSoon(aCallback);
    }, "sessionstore-state-write", false);

    
    let profilePath = Services.dirsvc.get("ProfD", Ci.nsIFile);
    let sessionStoreJS = profilePath.clone();
    sessionStoreJS.append("sessionstore.js");
    if (sessionStoreJS.exists())
      sessionStoreJS.remove(false);
    info("sessionstore.js was correctly removed: " + (!sessionStoreJS.exists()));

    
    
    Services.prefs.setIntPref("browser.sessionstore.interval", 0);
  }

  setupBlankState(function() {
    testOpenCloseWindow(false, TESTS[0], function() {
      testOpenCloseWindow(true, TESTS[1], function() {
        testOnWindow(false, TESTS[0].value, function() {
          testOnWindow(true, TESTS[0].value, finish);
        });
      });
    });
  });
}

function whenNewWindowLoaded(aIsPrivate, aCallback) {
  let win = OpenBrowserWindow({private: aIsPrivate});
  win.addEventListener("load", function onLoad() {
    win.removeEventListener("load", onLoad, false);
    aCallback(win);
  }, false);
}
