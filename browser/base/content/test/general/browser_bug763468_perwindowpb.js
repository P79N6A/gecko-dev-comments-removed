




function test() {
  
  waitForExplicitFinish();
  let windowsToClose = [];
  let newTab;
  let newTabURL;
  let mode;

  function doTest(aIsPrivateMode, aWindow, aCallback) {
    whenNewTabLoaded(aWindow, function () {
      if (aIsPrivateMode) {
        mode = "per window private browsing";
        newTabURL = "about:privatebrowsing";
      } else {
        mode = "normal";
        newTabURL = NewTabURL.get();
      }

      is(aWindow.gBrowser.currentURI.spec, newTabURL,
        "URL of NewTab should be " + newTabURL + " in " + mode +  " mode");

      aWindow.gBrowser.removeTab(aWindow.gBrowser.selectedTab);
      aCallback()
    });
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
