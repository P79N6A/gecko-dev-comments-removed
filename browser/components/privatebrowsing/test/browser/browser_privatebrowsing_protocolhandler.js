






function test() {
  
  waitForExplicitFinish();
  let windowsToClose = [];
  let notificationValue = "Protocol Registration: testprotocol";
  let testURI = "http://example.com/browser/" +
    "browser/components/privatebrowsing/test/browser/browser_privatebrowsing_protocolhandler_page.html";

  function doTest(aIsPrivateMode, aWindow, aCallback) {
    aWindow.gBrowser.selectedBrowser.addEventListener("load", function onLoad() {
      aWindow.gBrowser.selectedBrowser.removeEventListener("load", onLoad, true);

      setTimeout(function() {
        let notificationBox = aWindow.gBrowser.getNotificationBox();
        let notification = notificationBox.getNotificationWithValue(notificationValue);

        if (aIsPrivateMode) {
          
          ok(!notification, "Notification box should not be displayed inside of private browsing mode");
        } else {
          
          ok(notification, "Notification box should be displaying outside of private browsing mode");
        }

        aCallback();
      }, 100); 
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

  
  testOnWindow({}, function(aWin) {
    doTest(false, aWin, function() {
      
      testOnWindow({private: true}, function(aWin) {
        doTest(true, aWin, finish);
      });
    });
  });
}
