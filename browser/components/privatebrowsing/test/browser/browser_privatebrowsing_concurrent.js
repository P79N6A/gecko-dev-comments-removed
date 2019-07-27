












add_task(function test() {
  let prefix = 'http://mochi.test:8888/browser/browser/components/privatebrowsing/test/browser/browser_privatebrowsing_concurrent_page.html';

  gBrowser.selectedTab = gBrowser.addTab(prefix + '?action=set&name=test&value=value&initial=true');
  let non_private_tab = gBrowser.selectedBrowser;
  yield BrowserTestUtils.browserLoaded(non_private_tab);

  gBrowser.selectedTab = gBrowser.addTab();
  let private_tab = gBrowser.selectedBrowser;
  yield BrowserTestUtils.browserLoaded(private_tab);
  yield ContentTask.spawn(private_tab, {}, function* () {
    docShell.QueryInterface(Ci.nsILoadContext).usePrivateBrowsing = true;
  });
  private_tab.loadURI(prefix + '?action=set&name=test2&value=value2');
  yield BrowserTestUtils.browserLoaded(private_tab);

  non_private_tab.loadURI(prefix + '?action=get&name=test2');
  yield BrowserTestUtils.browserLoaded(non_private_tab);
  let elts = non_private_tab.contentTitle.split('|');
  isnot(elts[0], 'value2', "public window shouldn't see private storage");
  is(elts[1], '1', "public window should only see public items");

  private_tab.loadURI(prefix + '?action=get&name=test');
  yield BrowserTestUtils.browserLoaded(private_tab);
  elts = private_tab.contentTitle.split('|');
  isnot(elts[0], 'value', "private window shouldn't see public storage");
  is(elts[1], '1', "private window should only see private items");

  yield ContentTask.spawn(private_tab, {}, function* (){
    docShell.QueryInterface(Ci.nsILoadContext).usePrivateBrowsing = false;
  });
  yield new Promise(resolve => Cu.schedulePreciseGC(resolve));

  private_tab.loadURI(prefix + '?action=get&name=test2');
  yield BrowserTestUtils.browserLoaded(private_tab);
  elts = private_tab.contentTitle.split('|');
  isnot(elts[0], 'value2', "public window shouldn't see cleared private storage");
  is(elts[1], '1', "public window should only see public items");

  yield ContentTask.spawn(private_tab, {}, function* (){
    docShell.QueryInterface(Ci.nsILoadContext).usePrivateBrowsing = true;
  });

  private_tab.loadURI(prefix + '?action=set&name=test3&value=value3');
  BrowserTestUtils.browserLoaded(private_tab);
  elts = private_tab.contentTitle.split('|');
  is(elts[1], '1', "private window should only see new private items");

  
  non_private_tab.loadURI(prefix + '?final=true');
  yield BrowserTestUtils.browserLoaded(non_private_tab);
  gBrowser.removeCurrentTab();
  gBrowser.removeCurrentTab();
});
