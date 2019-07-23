




































function waitForBrowserState(aState, aSetStateCallback) {
  let os = Cc["@mozilla.org/observer-service;1"].
           getService(Ci.nsIObserverService);
  let observer = {
    observe: function(aSubject, aTopic, aData) {
      os.removeObserver(this, "sessionstore-browser-state-restored");
      executeSoon(aSetStateCallback);
    }
  };
  os.addObserver(observer, "sessionstore-browser-state-restored", false);
  let ss = Cc["@mozilla.org/browser/sessionstore;1"].
           getService(Ci.nsISessionStore);
  ss.setBrowserState(JSON.stringify(aState));
}

function test() {
  
  
  
  let ss = Cc["@mozilla.org/browser/sessionstore;1"].
           getService(Ci.nsISessionStore);
  let os = Cc["@mozilla.org/observer-service;1"].
           getService(Ci.nsIObserverService);
  let wm = Cc["@mozilla.org/appshell/window-mediator;1"].
           getService(Ci.nsIWindowMediator);
  waitForExplicitFinish();

  function browserWindowsCount() {
    let count = 0;
    let e = wm.getEnumerator("navigator:browser");
    while (e.hasMoreElements()) {
      if (!e.getNext().closed)
        ++count;
    }

    return count;
  }

  is(browserWindowsCount(), 1, "Only one browser window should be open initially");

  let blankState = {
    windows: [{
      tabs: [{ entries: [{ url: "about:blank" }] }],
      _closedTabs: []
    }],
    _closedWindows: []
  };

  
  let testState = {
    windows: [
      { tabs: [{ entries: [{ url: "http://example.com/" }] }], selected: 1 },
      { tabs: [{ entries: [{ url: "about:robots"        }] }], selected: 1 },
    ],
    
    
    selectedWindow: 1
  };

  waitForBrowserState(testState, function() {
    is(browserWindowsCount(), 2, "Two windows should exist at this point");

    
    function pollMostRecentWindow() {
      if (wm.getMostRecentWindow("navigator:browser") == window) {
        waitForBrowserState(blankState, function() {
          is(browserWindowsCount(), 1, "Only one window should exist after cleanup");
          ok(!window.closed, "Restoring the old state should have left this window open");
          finish();
        });
      }
      else {
        info("waiting for the current window to become active");
        setTimeout(pollMostRecentWindow, 0);
      }
    }
    pollMostRecentWindow();
  });
}
