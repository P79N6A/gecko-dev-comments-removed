




function test() {
  
  waitForExplicitFinish();
  let windowsToClose = [];
  let testURI =
    "http://example.com/tests/toolkit/components/satchel/test/subtst_privbrowsing.html";
  let formHistory = Cc["@mozilla.org/satchel/form-history;1"].
    getService(Ci.nsIFormHistory2);

  function doTest(aIsPrivateMode, aShouldValueExist, aWindow, aCallback) {
    aWindow.gBrowser.selectedBrowser.addEventListener("load", function onLoad() {
      aWindow.gBrowser.selectedBrowser.removeEventListener("load", onLoad, true);

      
      
      
      aWindow.gBrowser.selectedBrowser.addEventListener("load", function onLoad() {
        aWindow.gBrowser.selectedBrowser.removeEventListener("load", onLoad, true);
        executeSoon(aCallback);
      }, true);

      is(formHistory.entryExists("field", "value"), aShouldValueExist,
        "Checking value exists in form history");
    }, true);

    aWindow.gBrowser.selectedBrowser.loadURI(testURI);
  }

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


  testOnWindow({private: true}, function(aWin) {
    doTest(true, false, aWin, function() {
      
      
      testOnWindow({}, function(aWin) {
        doTest(false, false, aWin, function() {
          finish();
        });
      });
    });
  });
}
