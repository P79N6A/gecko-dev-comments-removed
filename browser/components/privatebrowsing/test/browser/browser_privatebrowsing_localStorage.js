



 add_task(function test() {
  requestLongerTimeout(2);
  const page1 = 'http://mochi.test:8888/browser/browser/components/privatebrowsing/test/browser/' +
                'browser_privatebrowsing_localStorage_page1.html'

  let win = yield BrowserTestUtils.openNewBrowserWindow({private: true});

  let tab = win.gBrowser.selectedTab = win.gBrowser.addTab(page1);
  let browser = win.gBrowser.selectedBrowser;
  yield BrowserTestUtils.browserLoaded(browser);

  browser.loadURI(
    'http://mochi.test:8888/browser/browser/components/privatebrowsing/test/browser/' +
    'browser_privatebrowsing_localStorage_page2.html');
  yield BrowserTestUtils.browserLoaded(browser);

  is(browser.contentTitle, '2', "localStorage should contain 2 items");

  
  yield BrowserTestUtils.closeWindow(win);
 });
