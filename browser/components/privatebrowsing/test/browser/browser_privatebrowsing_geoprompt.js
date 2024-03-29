






add_task(function* test() {
  const testPageURL = "http://mochi.test:8888/browser/" +
    "browser/components/privatebrowsing/test/browser/browser_privatebrowsing_geoprompt_page.html";

  function checkGeolocation(aPrivateMode, aWindow) {
    return Task.spawn(function* () {
      aWindow.gBrowser.selectedTab = aWindow.gBrowser.addTab(testPageURL);
      yield BrowserTestUtils.browserLoaded(aWindow.gBrowser.selectedBrowser);

      let notification = aWindow.PopupNotifications.getNotification("geolocation");

      
      while (!notification){
        yield new Promise(resolve => { executeSoon(resolve); });
        let notification = aWindow.PopupNotifications.getNotification("geolocation");
      }

      if (aPrivateMode) {
        
        is(notification.secondaryActions.length, 0, "Secondary actions shouldn't exist (always/never remember)");
      } else {
        ok(notification.secondaryActions.length > 1, "Secondary actions should exist (always/never remember)");
      }
      notification.remove();

      aWindow.gBrowser.removeCurrentTab();
    });
  };

  let win = yield BrowserTestUtils.openNewBrowserWindow();
  let browser = win.gBrowser.selectedBrowser;
  browser.loadURI(testPageURL);
  yield BrowserTestUtils.browserLoaded(browser);

  yield checkGeolocation(false, win);

  let privateWin = yield BrowserTestUtils.openNewBrowserWindow({private: true});
  let privateBrowser = privateWin.gBrowser.selectedBrowser;
  privateBrowser.loadURI(testPageURL);
  yield BrowserTestUtils.browserLoaded(privateBrowser);

  yield checkGeolocation(true, privateWin);

  
  yield BrowserTestUtils.closeWindow(win);
  yield BrowserTestUtils.closeWindow(privateWin);
});
