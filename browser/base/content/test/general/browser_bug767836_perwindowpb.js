



function test() {
  
  waitForExplicitFinish();
  let newTabPrefName = "browser.newtab.url";
  let newTabURL;
  let testURL = "http://example.com/";
  let mode;

  function doTest(aIsPrivateMode, aWindow, aCallback) {
    openNewTab(aWindow, function () {
      if (aIsPrivateMode) {
        mode = "per window private browsing";
        newTabURL = "about:privatebrowsing";
      } else {
        mode = "normal";
        newTabURL = Services.prefs.getCharPref(newTabPrefName) || "about:blank";
      }

      
      is(aWindow.gBrowser.selectedBrowser.currentURI.spec, newTabURL,
        "URL of NewTab should be " + newTabURL + " in " + mode +  " mode");
      
      Services.prefs.setCharPref(newTabPrefName, testURL);
      ok(Services.prefs.prefHasUserValue(newTabPrefName), "Custom newtab url is set");

      
      openNewTab(aWindow, function () {
        is(aWindow.gBrowser.selectedBrowser.currentURI.spec, testURL,
           "URL of NewTab should be the custom url");

        
        Services.prefs.clearUserPref(newTabPrefName);
        ok(!Services.prefs.prefHasUserValue(newTabPrefName), "No custom newtab url is set");

        aWindow.gBrowser.removeTab(aWindow.gBrowser.selectedTab);
        aWindow.gBrowser.removeTab(aWindow.gBrowser.selectedTab);
        aWindow.close();
        aCallback()
      });
    });
  }

  function testOnWindow(aIsPrivate, aCallback) {
    whenNewWindowLoaded({private: aIsPrivate}, function(win) {
      executeSoon(function() aCallback(win));
    });
  }

  
  ok(!Services.prefs.prefHasUserValue(newTabPrefName), "No custom newtab url is set");

  
  testOnWindow(false, function(aWindow) {
    doTest(false, aWindow, function() {
      
      testOnWindow(true, function(aWindow) {
        doTest(true, aWindow, function() {
          finish();
        });
      });
    });
  });
}

function openNewTab(aWindow, aCallback) {
  
  aWindow.BrowserOpenTab();

  let browser = aWindow.gBrowser.selectedBrowser;
  if (browser.contentDocument.readyState == "complete") {
    executeSoon(aCallback);
    return;
  }

  browser.addEventListener("load", function onLoad() {
    browser.removeEventListener("load", onLoad, true);
    executeSoon(aCallback);
  }, true);
}
