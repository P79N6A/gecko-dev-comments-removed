






add_task(function* test() {
  let notificationValue = "Protocol Registration: testprotocol";
  let testURI = "http://example.com/browser/" +
    "browser/components/privatebrowsing/test/browser/browser_privatebrowsing_protocolhandler_page.html";

  let doTest = Task.async(function* (aIsPrivateMode, aWindow) {
    let tab = aWindow.gBrowser.selectedTab = aWindow.gBrowser.addTab(testURI);
    yield BrowserTestUtils.browserLoaded(tab.linkedBrowser);

    let promiseFinished = PromiseUtils.defer();
    setTimeout(function() {
      let notificationBox = aWindow.gBrowser.getNotificationBox();
      let notification = notificationBox.getNotificationWithValue(notificationValue);

      if (aIsPrivateMode) {
        
        ok(!notification, "Notification box should not be displayed inside of private browsing mode");
      } else {
        
        ok(notification, "Notification box should be displaying outside of private browsing mode");
      }

      promiseFinished.resolve();
    }, 100); 

    yield promiseFinished.promise;
  });

  
  let win = yield BrowserTestUtils.openNewBrowserWindow();
  yield doTest(false, win);

  
  let privateWin = yield BrowserTestUtils.openNewBrowserWindow({private: true});
  yield doTest(true, privateWin);

  
  yield BrowserTestUtils.closeWindow(win);
  yield BrowserTestUtils.closeWindow(privateWin);
});
