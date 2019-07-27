



"use strict";



add_task(function* test_aboutprivatebrowsing_open_window() {
  let win = yield BrowserTestUtils.openNewBrowserWindow();
  let browser = win.gBrowser.addTab("about:privatebrowsing").linkedBrowser;
  yield BrowserTestUtils.browserLoaded(browser);

  let promiseWindowOpened = BrowserTestUtils.domWindowOpened();
  yield ContentTask.spawn(browser, {}, function* () {
    content.document.getElementById("startPrivateBrowsing").click();
  });
  let private_window = yield promiseWindowOpened;

  ok(PrivateBrowsingUtils.isWindowPrivate(private_window),
     "The opened window should be private.");

  
  yield BrowserTestUtils.closeWindow(private_window);
  yield BrowserTestUtils.closeWindow(win);
});
