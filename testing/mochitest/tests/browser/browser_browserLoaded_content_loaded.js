

'use strict';


function isDOMLoaded(aBrowser) {
  return aBrowser.contentWindowAsCPOW.document.readyState === 'complete';
}



add_task(function*() {
  let tab = gBrowser.addTab('http://example.com');
  let browser = tab.linkedBrowser;
  yield BrowserTestUtils.browserLoaded(browser);
  Assert.ok(isDOMLoaded(browser), 'browser', 'Expect browser to have loaded.');
  gBrowser.removeTab(tab);
});



add_task(function*() {
  let tabURLs = [
    `http://example.org`,
    `http://mochi.test:8888`,
    `http://test:80`,
  ];
  
  let browsers = [
    for (u of tabURLs) gBrowser.addTab(u).linkedBrowser
  ];
  
  yield Promise.all((
    for (b of browsers) BrowserTestUtils.browserLoaded(b)
  ));
  let expected = 'Expected all promised browsers to have loaded.';
  Assert.ok(browsers.every(isDOMLoaded), expected);
  
  browsers
    .map(browser => gBrowser.getTabForBrowser(browser))
    .forEach(tab => gBrowser.removeTab(tab));
});
