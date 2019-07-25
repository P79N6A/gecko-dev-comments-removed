




































function browserWindowsCount() {
  let count = 0;
  let e = Services.wm.getEnumerator("navigator:browser");
  while (e.hasMoreElements()) {
    if (!e.getNext().closed)
      ++count;
  }
  return count;
}

function test() {
  
  is(browserWindowsCount(), 1, "Only one browser window should be open initially");

  waitForExplicitFinish();
  requestLongerTimeout(2);

  let ss = Cc["@mozilla.org/browser/sessionstore;1"].
           getService(Ci.nsISessionStore);

  
  
  
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
    gBrowser.stop();

    let uris = [];
    for (let i = 0; i < 25; i++)
      uris.push("http://example.com/" + i);

    
    
    
    
    gBrowser.addTabsProgressListener({
      onLocationChange: function (aBrowser) {
        if (uris.indexOf(aBrowser.currentURI.spec) > -1) {
          gBrowser.removeTabsProgressListener(this);
          firstLocationChange();
        }
      }
    });

    function firstLocationChange() {
      let state = JSON.parse(ss.getBrowserState());
      let hasUTV = state.windows[0].tabs.some(function(aTab) {
        return aTab.userTypedValue && aTab.userTypedClear && !aTab.entries.length;
      });

      ok(hasUTV, "At least one tab has a userTypedValue with userTypedClear with no loaded URL");

      gBrowser.addEventListener("load", firstLoad, true);
    }

    function firstLoad() {
      gBrowser.removeEventListener("load", firstLoad, true);

      let state = JSON.parse(ss.getBrowserState());
      let hasSH = state.windows[0].tabs.some(function(aTab) {
        return !("userTypedValue" in aTab) && aTab.entries[0].url;
      });

      ok(hasSH, "At least one tab has its entry in SH");

      runNextTest();
    }

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

      gURLBar.value = "example.org";
      let event = document.createEvent("Events");
      event.initEvent("input", true, false);
      gURLBar.dispatchEvent(event);

      executeSoon(function() {
        is(browser.userTypedValue, "example.org",
           "userTypedValue was set when changing gURLBar.value");
        is(browser.userTypedClear, 0,
           "userTypedClear was not changed when changing gURLBar.value");

        
        let newState = JSON.parse(ss.getBrowserState());
        is(newState.windows[0].tabs[0].userTypedValue, "example.org",
           "sessionstore got correct userTypedValue");
        is(newState.windows[0].tabs[0].userTypedClear, 0,
           "sessionstore got correct userTypedClear");
        runNextTest();
      });
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

    waitForBrowserState(state, function() {
      let browser = gBrowser.selectedBrowser;
      is(browser.currentURI.spec, "http://example.com/",
         "userTypedClear=2 caused userTypedValue to be loaded");
      is(browser.userTypedValue, null,
         "userTypedValue was null after loading a URI");
      is(browser.userTypedClear, 0,
         "userTypeClear reset to 0");
      is(gURLBar.value, gURLBar.trimValue("http://example.com/"),
         "Address bar's value set after loading URI");
      runNextTest();
    });
  }


  let tests = [test_newTabFocused, test_newTabNotFocused,
               test_existingSHEnd_noClear, test_existingSHMiddle_noClear,
               test_getBrowserState_lotsOfTabsOpening,
               test_getBrowserState_userTypedValue, test_userTypedClearLoadURI];
  let originalState = ss.getBrowserState();
  let state = {
    windows: [{
      tabs: [{ entries: [{ url: "about:blank" }] }]
    }]
  };
  function runNextTest() {
    if (tests.length) {
      waitForBrowserState(state, tests.shift());
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
