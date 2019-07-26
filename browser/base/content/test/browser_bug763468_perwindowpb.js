




function test() {
  
  waitForExplicitFinish();
  let windowsToClose = [];
  let newTab;
  let newTabPrefName = "browser.newtab.url";
  let newTabURL;
  let mode;

  function doTest(aIsPrivateMode, aWindow, aCallback) {
    aWindow.BrowserOpenTab();
    aWindow.gBrowser.selectedTab.linkedBrowser.addEventListener("load", function onLoad() {
      aWindow.gBrowser.selectedTab.linkedBrowser.removeEventListener("load", onLoad, true);
      if (aIsPrivateMode) {
        mode = "per window private browsing";
        newTabURL = "about:privatebrowsing";
      } else {
        mode = "normal";
        newTabURL = Services.prefs.getCharPref(newTabPrefName) || "about:blank";
      }

      is(aWindow.gBrowser.currentURI.spec, newTabURL,
        "URL of NewTab should be " + newTabURL + " in " + mode +  " mode");

      aWindow.gBrowser.removeTab(aWindow.gBrowser.selectedTab);
      aCallback()
    }, true);
  };

  function testOnWindow(aOptions, aCallback) {
    whenNewWindowLoaded(aOptions, function(aWin) {
      windowsToClose.push(aWin);
      
      
      
      executeSoon(function() aCallback(aWin));
    });
  };

   
  registerCleanupFunction(function() {
    windowsToClose.forEach(function(aWin) {
      aWin.close();
    });
  });

  
  testOnWindow({}, function(aWin) {
    doTest(false, aWin, function() {
      
      testOnWindow({private: true}, function(aWin) {
        doTest(true, aWin, function() {
          
          testOnWindow({}, function(aWin) {
            doTest(false, aWin, finish);
          });
        });
      });
    });
  });
}
