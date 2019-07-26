











function test() {
  
  waitForExplicitFinish();
  let windowsToClose = [];
  let testURI = "about:blank";
  let prefix = 'http://mochi.test:8888/browser/browser/components/privatebrowsing/test/browser/';

  function doTest(aIsPrivateMode, aWindow, aCallback) {
    aWindow.gBrowser.selectedBrowser.addEventListener("load", function onLoad() {
      aWindow.gBrowser.selectedBrowser.removeEventListener("load", onLoad, true);

      if (aIsPrivateMode) {
        
        is(aWindow.gBrowser.contentWindow.document.title, '1', "localStorage should contain 1 item");
      } else {
        
        is(aWindow.gBrowser.contentWindow.document.title, 'null|0', 'localStorage should contain 0 items');
      }

      aCallback();
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
    testURI = prefix + 'browser_privatebrowsing_localStorage_before_after_page.html';
    doTest(true, aWin, function() {
      
      testOnWindow({}, function(aWin) {
        testURI = prefix + 'browser_privatebrowsing_localStorage_before_after_page2.html';
        doTest(false, aWin, finish);
      });
    });
  });
}
