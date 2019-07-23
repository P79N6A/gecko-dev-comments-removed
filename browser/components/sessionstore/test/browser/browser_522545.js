




































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

  let ss = Cc["@mozilla.org/browser/sessionstore;1"].
           getService(Ci.nsISessionStore);
  let os = Cc["@mozilla.org/observer-service;1"].
           getService(Ci.nsIObserverService);

  function waitForBrowserState(aState, aSetStateCallback) {
    let observer = {
      observe: function(aSubject, aTopic, aData) {
        os.removeObserver(this, "sessionstore-browser-state-restored");
        executeSoon(aSetStateCallback);
      }
    };
    os.addObserver(observer, "sessionstore-browser-state-restored", false);
    ss.setBrowserState(JSON.stringify(aState));
  }

  
  
  
  function test_newTabFocused() {
    let state = {
      windows: [{
        tabs: [
          { entries: [{ url: "about:mozilla" }] },
          { entries: [], userTypedValue: "example.com", userTypedClear: 0 }
        ],
        selected: 2
      }]
    };

    waitForBrowserState(state, function() {
      let browser = gBrowser.selectedBrowser;
      is(browser.currentURI.spec, "about:blank",
         "No history entries still sets currentURI to about:blank");
      is(browser.userTypedValue, "example.com",
         "userTypedValue was correctly restored");
      is(browser.userTypedClear, 0,
         "userTypeClear restored as expected");
      is(gURLBar.value, "example.com",
         "Address bar's value correctly restored");
      
      gBrowser.selectedTab = gBrowser.tabContainer.getItemAtIndex(0);
      is(gURLBar.value, "about:mozilla",
         "Address bar's value correctly updated");
      runNextTest();
    });
  }

  
  
  
  function test_newTabNotFocused() {
    let state = {
      windows: [{
        tabs: [
          { entries: [{ url: "about:mozilla" }] },
          { entries: [], userTypedValue: "example.org", userTypedClear: 0 }
        ],
        selected: 1
      }]
    };

    waitForBrowserState(state, function() {
      let browser = gBrowser.getBrowserAtIndex(1);
      is(browser.currentURI.spec, "about:blank",
         "No history entries still sets currentURI to about:blank");
      is(browser.userTypedValue, "example.org",
         "userTypedValue was correctly restored");
      is(browser.userTypedClear, 0,
         "userTypeClear restored as expected");
      is(gURLBar.value, "about:mozilla",
         "Address bar's value correctly restored");
      
      gBrowser.selectedTab = gBrowser.tabContainer.getItemAtIndex(1);
      is(gURLBar.value, "example.org",
         "Address bar's value correctly updated");
      runNextTest();
    });
  }

  
  
  
  function test_existingSHEnd_noClear() {
    let state = {
      windows: [{
        tabs: [{
          entries: [{ url: "about:mozilla" }, { url: "about:config" }],
          index: 2,
          userTypedValue: "example.com",
          userTypedClear: 0
        }]
      }]
    };

    waitForBrowserState(state, function() {
      let browser = gBrowser.selectedBrowser;
      is(browser.currentURI.spec, "about:config",
         "browser.currentURI set to current entry in SH");
      is(browser.userTypedValue, "example.com",
         "userTypedValue was correctly restored");
      is(browser.userTypedClear, 0,
         "userTypeClear restored as expected");
      is(gURLBar.value, "example.com",
         "Address bar's value correctly restored to userTypedValue");
      runNextTest();
    });
  }

  
  
  
  function test_existingSHMiddle_noClear() {
    let state = {
      windows: [{
        tabs: [{
          entries: [{ url: "about:mozilla" }, { url: "about:config" }],
          index: 1,
          userTypedValue: "example.org",
          userTypedClear: 0
        }]
      }]
    };

    waitForBrowserState(state, function() {
      let browser = gBrowser.selectedBrowser;
      is(browser.currentURI.spec, "about:mozilla",
         "browser.currentURI set to current entry in SH");
      is(browser.userTypedValue, "example.org",
         "userTypedValue was correctly restored");
      is(browser.userTypedClear, 0,
         "userTypeClear restored as expected");
      is(gURLBar.value, "example.org",
         "Address bar's value correctly restored to userTypedValue");
      runNextTest();
    });
  }

  
  function test_getBrowserState_lotsOfTabsOpening() {
    let uris = [];
    for (let i = 0; i < 25; i++)
      uris.push("http://example.com/" + i);

    
    
    
    
    gBrowser.addEventListener("load", function(aEvent) {
      if (gBrowser.currentURI.spec == "about:blank")
        return;
      gBrowser.removeEventListener("load", arguments.callee, true);

      let state = JSON.parse(ss.getBrowserState());

      let hasSH = state.windows[0].tabs.some(function(aTab) {
        return !("userTypedValue" in aTab) && aTab.entries[0].url;
      });
      let hasUTV = state.windows[0].tabs.some(function(aTab) {
        return aTab.userTypedValue && aTab.userTypedClear && !aTab.entries.length;
      });

      ok(hasSH, "At least one tab has it's entry in SH");
      ok(hasUTV, "At least one tab has a userTypedValue with userTypedClear with no loaded URL");

      runNextTest();

    }, true);
    gBrowser.loadTabs(uris);
  }

  
  
  function test_getBrowserState_userTypedValue() {
    let state = {
      windows: [{
        tabs: [{ entries: [] }]
      }]
    };

    waitForBrowserState(state, function() {
      let browser = gBrowser.selectedBrowser;
      
      is(browser.userTypedValue, null, "userTypedValue is empty to start");
      is(browser.userTypedClear, 0, "userTypedClear is 0 to start");

      gURLBar.value = "mozilla.org";
      let event = document.createEvent("Events");
      event.initEvent("input", true, false);
      gURLBar.dispatchEvent(event);

      is(browser.userTypedValue, "mozilla.org",
         "userTypedValue was set when changing gURLBar.value");
      is(browser.userTypedClear, 0,
         "userTypedClear was not changed when changing gURLBar.value");

      
      let newState = JSON.parse(ss.getBrowserState());
      is(newState.windows[0].tabs[0].userTypedValue, "mozilla.org",
         "sessionstore got correct userTypedValue");
      is(newState.windows[0].tabs[0].userTypedClear, 0,
         "sessionstore got correct userTypedClear");
      runNextTest();
    });
  }

  
  
  
  function test_userTypedClearLoadURI() {
    let state = {
      windows: [{
        tabs: [
          { entries: [], userTypedValue: "http://example.com", userTypedClear: 2 }
        ]
      }]
    };

    
    
    
    
    ss.setBrowserState(JSON.stringify(state));
    gBrowser.addEventListener("load", function(aEvent) {
      if (gBrowser.currentURI.spec == "about:blank")
        return;
      gBrowser.removeEventListener("load", arguments.callee, true);

      let browser = gBrowser.selectedBrowser;
      is(browser.currentURI.spec, "http://example.com/",
         "userTypedClear=2 caused userTypedValue to be loaded");
      is(browser.userTypedValue, null,
         "userTypedValue was null after loading a URI");
      is(browser.userTypedClear, 0,
         "userTypeClear reset to 0");
      is(gURLBar.value, "http://example.com/",
         "Address bar's value set after loading URI");
      runNextTest();
    }, true);
  }


  let tests = [test_newTabFocused, test_newTabNotFocused,
               test_existingSHEnd_noClear, test_existingSHMiddle_noClear,
               test_getBrowserState_lotsOfTabsOpening,
               test_getBrowserState_userTypedValue, test_userTypedClearLoadURI];
  let originalState = ss.getBrowserState();
  function runNextTest() {
    if (tests.length) {
      tests.shift().call();
    } else {
      ss.setBrowserState(originalState);
      executeSoon(function () {
        is(browserWindowsCount(), 1, "Only one browser window should be open eventually");
        finish();
      });
    }
  }

  
  runNextTest();
}
