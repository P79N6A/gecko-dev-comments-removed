












add_task(function test() {
  let prefix = 'http://mochi.test:8888/browser/browser/components/privatebrowsing/test/browser/browser_privatebrowsing_concurrent_page.html';

  function setUsePrivateBrowsing(browser, val) {
    return ContentTask.spawn(browser, val, function* (val) {
      docShell.QueryInterface(Ci.nsILoadContext).usePrivateBrowsing = val;
    });
  };

  function getElts(browser) {
    return browser.contentTitle.split('|');
  };

  
  gBrowser.selectedTab = gBrowser.addTab(prefix + '?action=set&name=test&value=value&initial=true');
  let non_private_browser = gBrowser.selectedBrowser;
  yield BrowserTestUtils.browserLoaded(non_private_browser);


  
  gBrowser.selectedTab = gBrowser.addTab();
  let private_browser = gBrowser.selectedBrowser;
  yield BrowserTestUtils.browserLoaded(private_browser);
  yield setUsePrivateBrowsing(private_browser, true);
  private_browser.loadURI(prefix + '?action=set&name=test2&value=value2');
  yield BrowserTestUtils.browserLoaded(private_browser);


  
  non_private_browser.loadURI(prefix + '?action=get&name=test2');
  yield BrowserTestUtils.browserLoaded(non_private_browser);
  let elts = yield getElts(non_private_browser);
  isnot(elts[0], 'value2', "public window shouldn't see private storage");
  is(elts[1], '1', "public window should only see public items");


  
  private_browser.loadURI(prefix + '?action=get&name=test');
  yield BrowserTestUtils.browserLoaded(private_browser);
  elts = yield getElts(private_browser);
  isnot(elts[0], 'value', "private window shouldn't see public storage");
  is(elts[1], '1', "private window should only see private items");


  
  
  yield setUsePrivateBrowsing(private_browser, false);
  yield new Promise(resolve => Cu.schedulePreciseGC(resolve));

  private_browser.loadURI(prefix + '?action=get&name=test2');
  yield BrowserTestUtils.browserLoaded(private_browser);
  elts = yield getElts(private_browser);
  isnot(elts[0], 'value2', "public window shouldn't see cleared private storage");
  is(elts[1], '1', "public window should only see public items");


  
  
  yield setUsePrivateBrowsing(private_browser, true);

  private_browser.loadURI(prefix + '?action=set&name=test3&value=value3');
  BrowserTestUtils.browserLoaded(private_browser);
  elts = yield getElts(private_browser);
  is(elts[1], '1', "private window should only see new private items");

  
  non_private_browser.loadURI(prefix + '?final=true');
  yield BrowserTestUtils.browserLoaded(non_private_browser);
  gBrowser.removeCurrentTab();
  gBrowser.removeCurrentTab();
});
