











add_task(function test() {
  let testURI = "about:blank";
  let prefix = 'http://mochi.test:8888/browser/browser/components/privatebrowsing/test/browser/';

  
  let privateWin = yield BrowserTestUtils.openNewBrowserWindow({private: true});
  let privateBrowser = privateWin.gBrowser.addTab(
    prefix + 'browser_privatebrowsing_localStorage_before_after_page.html').linkedBrowser;
  yield BrowserTestUtils.browserLoaded(privateBrowser);

  is(privateBrowser.contentTitle, '1', "localStorage should contain 1 item");

  
  let win = yield BrowserTestUtils.openNewBrowserWindow();
  let browser = win.gBrowser.addTab(
    prefix + 'browser_privatebrowsing_localStorage_before_after_page2.html').linkedBrowser;
  yield BrowserTestUtils.browserLoaded(browser);

  is(browser.contentTitle, 'null|0', 'localStorage should contain 0 items');

  
  yield BrowserTestUtils.closeWindow(privateWin);
  yield BrowserTestUtils.closeWindow(win);
});
